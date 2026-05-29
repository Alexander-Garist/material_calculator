#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// Характеристики конкретного материала
struct Material
{
	std::string name;	// Название

	int width;		// Ширина 1 куска материала
	int height;	// Высота 1 куска материала

	bool can_rotate;	// Флаг возможности поворота куска материала

    // ДОБАВИТЬ ЭТОТ ОПЕРАТОР:
    // Он сравнивает материалы по имени. Если имена равны, мапа считает их одним материалом.
    bool operator<(const Material& other) const 
    {
        return name < other.name;
    }
};

// Характеристики конкретной зоны отделки в пределах одной стены
struct FinishZone
{
	// Расположение зоны отдлеки на стене
	int offset_X;			// Смещение зоны отделки по горизонтали от левого края
	int offset_Y;			// Смещение зоны отделки по вертикали от пола

	// Размеры зоны отделки
	int width;		// Ширина зоны отделки
	int height;	// Высота зоны отделки

	// Используемый материал в этой зоне отделки
	Material material;
};

// Варианты края стены
enum class CornerType
{
    None,   // Нет угла ИЛИ угол не 90 градусов, т.к. угловой профиль может использоваться только для прямого угла между стенами
    Inner,  // Внутренний угол
    Outer   // Внешний угол
};

// Стена класс, потому что содержит логику работы с ней
class Wall
{
private:
	int Wall_ID;
	int Wall_Width;		// Ширина стены
	int Wall_Height;	// Высота стены

	std::vector<FinishZone> Wall_Finish_Zones;	// Список зон отделки на конкретной стене

    // Каким типом угла эта стена стыкуется со СЛЕДУЮЩЕЙ стеной
    CornerType Next_Corner_Type = CornerType::None;

public:
	// Конструктор создания новой стены
	Wall(int id, int width, int height) : Wall_ID(id), Wall_Width(width), Wall_Height(height) {}

	// Метод проверки на пересечение 2 зон отделки
	bool isOverlapping(const FinishZone& zone_1, const FinishZone& zone_2) const
	{
		return !((zone_1.offset_X + zone_1.width <= zone_2.offset_X) ||
			(zone_2.offset_X + zone_2.width <= zone_1.offset_X) ||
			(zone_1.offset_Y + zone_1.height <= zone_2.offset_Y) ||
			(zone_2.offset_Y + zone_2.height <= zone_1.offset_Y));
	}

	// Метод для добавления зоны отделки на стену
	bool AddFinishZone(int offset_x, int offset_y, int width, int height, Material material)
	{
		// Создается временная зона отделки для проверок
		FinishZone new_zone{offset_x, offset_y, width, height, material};

		if (offset_x < 0 || offset_y < 0 || width <= 0 || height <= 0)
		{
			std::cout << "[Ошибка] Некорректные размеры или координаты зоны.\n";
			return false;
		}
				
		if (offset_x + width > Wall_Width || offset_y + height > Wall_Height) 
		{
			std::cout << "[Ошибка] Зона выходит за физические границы стены!\n";
			return false;
		}
		
		for (const auto& existing_zone : Wall_Finish_Zones) 
		{
			if (isOverlapping(new_zone, existing_zone)) 
			{
				std::cout << "[Ошибка] Новая зона пересекается с уже существующей зоной!\n";
				return false;
			}
		}

		// Если все проверки пройдены, сохраняем зону
		Wall_Finish_Zones.push_back({ offset_x, offset_y, width, height, material });
		std::cout << "[Успех] Зона отделки успешно добавлена на стену [" << Wall_ID << "].\n";
		return true;
	}

	// Геттер для получения всех зон отделки на стене
	const std::vector<FinishZone>& getFinishZones() const	{ return Wall_Finish_Zones;	}

	// Геттер для получения ID стены
	int getID() const	{ return Wall_ID; }

    // Сеттер для установки типа угла
    void SetNextCornerType(CornerType type) 
    {
        Next_Corner_Type = type;
    }

    // Геттер для типа угла
    CornerType getNextCornerType() const { return Next_Corner_Type; }

    // Геттер для высоты стены
    int getHeight() const { return Wall_Height; }
};

// Структура обрезка не содержит поля Material, т.к. расчет происходит по очереди для каждого материала
// сначала всё дерево, потом весь металл и т.д. => обрезки точно не перемешаются
struct Scrap
{
	int width;
	int height;
};

// Итоговый расчет по одному конкретному материалу
struct Material_Calculation_Result
{
	std::string material_name;		// Название материала
	int ones_used = 0;				// Количество использованного материала, шт.
	std::vector<Scrap> scraps;		// Список оставшихся обрезков
};



enum class ProfileType
{
    Edge,         // Торцевой
    Connecting,   // Соединительный
    InnerCorner,  // Угловой внутренний
    OuterCorner   // Угловой внешний
};

struct ProfileMaterial
{
    std::string name;
    int length;
    ProfileType type;
    bool has_LED; // Флаг наличия подсветки (например, светодиодный профиль)

    bool operator<(const ProfileMaterial& other) const
    {
        return name < other.name;
    }
};





class MaterialCalculator 
{
private:
    // Внутренний метод: считает только ОДИН конкретный материал по всем зонам отделки, где он используется
    Material_Calculation_Result CalculateSingleMaterial(const Material& material, const std::vector<FinishZone>& zones) 
    {
        Material_Calculation_Result result;     // Результат по одному материалу (структура ИМЯ|КОЛИЧЕСТВО|СПИСОК ОБРЕЗКОВ)
        result.material_name = material.name;   // Результат по конкретному материалу сразу получает название материала
       
        if (zones.empty()) return result;       // Проверка на отсутствие зон отделки заданным материалом

        // Локальный склад обрезков для текущего материала
        std::vector<Scrap> current_scraps;

        // Проходим циклом по всем зонам этого материала
        for (const auto& zone : zones) 
        {
            // Кроим зону по рядам: снизу вверх (по высоте)
            for (int y = 0; y < zone.height; y += material.height)
            {
                // Вычисляем высоту текущего ряда (обычно равна высоте листа, но самый верхний ряд может быть уже)
                int current_row_H = std::min(material.height, zone.height - y);

                // Идем по ширине текущего ряда: слева направо
                for (int x = 0; x < zone.width; ) 
                {
                    int current_needed_W = zone.width - x;

                    bool found_scrap = false;
                    size_t scrap_index = 0;

                    // 1. Ищем подходящий кусок на складе обрезков
                    for (size_t i = 0; i < current_scraps.size(); ++i) 
                    {
                        // Проверка без поворота куска
                        if (current_scraps[i].width >= current_needed_W && current_scraps[i].height >= current_row_H) 
                        {
                            found_scrap = true;
                            scrap_index = i;
                            break;
                        }
                        // Проверка с поворотом на 90 градусов (если материал это позволяет)
                        if (material.can_rotate && current_scraps[i].height >= current_needed_W && current_scraps[i].width >= current_row_H) 
                        {
                            std::swap(current_scraps[i].width, current_scraps[i].height); // поворачиваем
                            found_scrap = true;
                            scrap_index = i;
                            break;
                        }
                    }

                    int current_piece_W = 0;
                    int current_piece_H = 0;

                    if (found_scrap) 
                    {
                        // Забираем размеры обрезка
                        current_piece_W = current_scraps[scrap_index].width;

                        // ВАЖНО: Если обрезок по высоте больше, чем текущий ряд, 
                        // мы ОТРЕЗАЕМ от него верхнюю часть по всей его ширине и сразу возвращаем на склад!
                        if (current_scraps[scrap_index].height > current_row_H) 
                        {
                            current_scraps.push_back({ current_piece_W, current_scraps[scrap_index].height - current_row_H });
                            current_piece_H = current_row_H; // Теперь наш рабочий кусок строго равен высоте ряда
                        }
                        else 
                        {
                            current_piece_H = current_scraps[scrap_index].height;
                        }

                        // Удаляем старый целый обрезок со склада
                        current_scraps.erase(current_scraps.begin() + scrap_index);
                    }
                    else 
                    {
                        // Если обрезков нет — берем новый целый лист
                        result.ones_used++;
                        current_piece_W = material.width;
                        current_piece_H = material.height;
                    }

                    // Вычисляем, какую часть листа/обрезка мы реально уложили на стену
                    int used_W = std::min(current_piece_W, current_needed_W);
                    int used_H = std::min(current_piece_H, current_row_H);

                    // Лист отрезается по прямой линии (гильотинный рез). 
                    // Считаем новые обрезки, которые получились от текущего куска:
                    if (current_piece_W > used_W) 
                    {
                        current_scraps.push_back({ current_piece_W - used_W, current_piece_H });
                    }
                    if (current_piece_H > used_H)
                    {
                        current_scraps.push_back({ used_W, current_piece_H - used_H });
                    }

                    // Шагаем вперед по ширине стены на величину уложенного куска
                    x += used_W;
                }
            }
        }

        // Все неиспользованные обрезки этого материала сохраняем в итоговую структуру
        result.scraps = current_scraps;
        return result;
    }

    // Универсальный математический движок (1D-раскрой), который считает ШТУКИ из отрезков.
    // Ему всё равно, какой это профиль — угловой или торцевой. Он просто режет хлысты.
    int Calculate1DMinPieces(const std::vector<int>& required_cuts, int profile_length) 
    {
        if (required_cuts.empty()) return 0;

        std::vector<int> sorted_cuts = required_cuts;
        std::sort(sorted_cuts.begin(), sorted_cuts.end(), std::greater<int>());

        int pieces_used = 0;
        std::vector<int> scraps;
        int min_useful_length = profile_length / 3; // 1/3 от хлыста

        for (int cut : sorted_cuts)
        {
            while (cut > profile_length) 
            {
                pieces_used++;
                cut -= profile_length;
            }

            bool found = false;
            for (size_t i = 0; i < scraps.size(); ++i) 
            {
                if (scraps[i] >= cut) 
                {
                    scraps[i] -= cut;
                    if (scraps[i] < min_useful_length) 
                    {
                        scraps.erase(scraps.begin() + i);
                    }
                    found = true;
                    break;
                }
            }

            if (!found) 
            {
                pieces_used++;
                int rem = profile_length - cut;
                if (rem >= min_useful_length) 
                {
                    scraps.push_back(rem);
                }
            }
        }
        return pieces_used;
    }


public:
    // Главный публичный метод. Принимает список всех стен помещения.
    std::vector<Material_Calculation_Result> CalculateAllWalls(const std::vector<Wall>& room_walls) 
    {
        // Мапа для группировки: ключ - имя материала, значение - вектор его зон
        std::map<Material, std::vector<FinishZone>> grouped_zones;

        // ЭТАП 1: Группировка. Проходим по всем стенам и распределяем зоны по "коробкам"
        for (const auto& wall : room_walls) 
        {
            for (const auto& zone : wall.getFinishZones()) 
            {
                // Если ключа с таким именем материала еще нет в мапе, 
                // C++ сам создаст его и добавит зону в вектор.
                grouped_zones[zone.material].push_back(zone);
            }
        }

        // Вектор, где будут лежать финальные отчеты по каждому материалу
        std::vector<Material_Calculation_Result> final_results;

        // ЭТАП 2: Расчет. Итерируемся по нашей мапе.
        // пара: kv.first — это имя материала (string), kv.second — это вектор его зон
        for (const auto& kv : grouped_zones)
        {
            // Запускаем детальный алгоритм раскроя для накопленных зон конкретного материала
            Material_Calculation_Result mat_res = CalculateSingleMaterial(kv.first, kv.second);
            final_results.push_back(mat_res);
        }

        // Возвращаем собранный массив результатов в функцию main или в будущий GUI
        return final_results;
    }

    // Единый публичный метод для расчета КОНКРЕТНОГО профиля.
    // Он принимает профиль, список стен и вектор внутренних швов, которые накопились при расчете панелей.
    Material_Calculation_Result CalculateProfile(const ProfileMaterial& profile,
        const std::vector<Wall>& room_walls,
        const std::vector<int>& internal_panel_seams) 
    {
        Material_Calculation_Result result;
        result.material_name = profile.name + (profile.has_LED ? " (с подсветкой)" : "");

        std::vector<int> required_cuts;

        if (profile.type == ProfileType::Edge) 
        {
            // Логика торцевого профиля (без изменений)
            for (const auto& wall : room_walls) 
            {
                for (const auto& zone : wall.getFinishZones()) 
                {
                    required_cuts.push_back(zone.width);
                    required_cuts.push_back(zone.width);
                    required_cuts.push_back(zone.height);
                    required_cuts.push_back(zone.height);
                }
            }
        }
        else if (profile.type == ProfileType::Connecting) 
        {
            // Логика соединительного профиля (без изменений)
            required_cuts = internal_panel_seams;
        }
        else if (profile.type == ProfileType::InnerCorner) 
        {
            // Собираем высоты только тех стыков стен, которые помечены как ВНУТРЕННИЕ
            for (const auto& wall : room_walls) 
            {
                if (wall.getNextCornerType() == CornerType::Inner)
                {
                    required_cuts.push_back(wall.getHeight());
                }
            }
        }
        else if (profile.type == ProfileType::OuterCorner) 
        {
            // Собираем высоты только тех стыков стен, которые помечены как ВНЕШНИЕ
            for (const auto& wall : room_walls) 
            {
                if (wall.getNextCornerType() == CornerType::Outer) 
                {
                    required_cuts.push_back(wall.getHeight());
                }
            }
        }

        // Передаем собранные отрезки в единый математический движок
        result.ones_used = Calculate1DMinPieces(required_cuts, profile.length);

        return result;
    }
};


















// 4. КОНСОЛЬНЫЙ ИНТЕРФЕЙС (MAIN)
int main() 
{
    setlocale(LC_ALL, "ru");

    // База материалов панелей по умолчанию
    std::vector<Material> materials_db = {
        {"Панель (2800x1220, поворотный)", 1220, 2800, true},
        {"Рейка (3000x100, НЕ поворотная)", 100, 3000, false}
    };

    // База доступных профилей по умолчанию (для демонстрации в калькуляторе)
    std::vector<ProfileMaterial> profiles_db = {
        {"Торцевой П-профиль (3000мм)", 3000, ProfileType::Edge, false},
        {"Соединительный Н-профиль (3000мм)", 3000, ProfileType::Connecting, false},
        {"Угловой внутренний профиль (3000мм)", 3000, ProfileType::InnerCorner, false},
        {"Угловой внешний профиль (3000мм)", 3000, ProfileType::OuterCorner, false}
    };

    std::vector<Wall> room_walls;
    int wall_counter = 1;

    std::cout << "==================================================\n";
    std::cout << " Программа расчета листовых материалов и профилей\n";
    std::cout << "==================================================\n\n";

    while (true) {
        int wall_w, wall_h;
        std::cout << "--- Создание Стены [" << wall_counter << "] ---\n";
        std::cout << "Введите ширину стены в мм (или 0 для завершения ввода): ";
        std::cin >> wall_w;
        if (wall_w <= 0) break;

        std::cout << "Введите высоту стены в мм: ";
        std::cin >> wall_h;
        if (wall_h <= 0) {
            std::cout << "[Ошибка] Высота должна быть больше 0. Попробуйте снова.\n";
            continue;
        }

        Wall current_wall(wall_counter, wall_w, wall_h);
        std::cout << "[Успех] Создана стена " << wall_w << "x" << wall_h << " мм.\n\n";

        // Цикл добавления зон отделки на текущую стену
        while (true) {
            std::cout << "  Добавление зоны отделки для Стены №" << wall_counter << "\n";
            std::cout << "  Доступные материалы:\n";
            for (size_t i = 0; i < materials_db.size(); ++i) {
                std::cout << "    " << i + 1 << ". " << materials_db[i].name
                    << " [" << materials_db[i].width << "x" << materials_db[i].height << " мм]\n";
            }
            std::cout << "    0. Завершить отделку этой стены\n";

            int mat_choice;
            std::cout << "  Выберите номер материала: ";
            std::cin >> mat_choice;
            if (mat_choice <= 0 || mat_choice > static_cast<int>(materials_db.size())) break;

            Material selected_mat = materials_db[mat_choice - 1];

            int z_x, z_y, z_w, z_h;
            std::cout << "  Введите смещение X от левого края (мм): "; std::cin >> z_x;
            std::cout << "  Введите смещение Y от пола (мм): ";        std::cin >> z_y;
            std::cout << "  Введите ширину зоны (мм): ";               std::cin >> z_w;
            std::cout << "  Введите высоту зоны (мм): ";               std::cin >> z_h;

            current_wall.AddFinishZone(z_x, z_y, z_w, z_h, selected_mat);
            std::cout << "\n";
        }

        // --- НОВЫЙ БЛОК: Определение стыков и углов для профилей ---
        std::cout << "  --- Настройка стыка в конце Стены №" << wall_counter << " ---\n";
        std::cout << "  Как эта стена стыкуется со следующей?\n";
        std::cout << "    1. Образует ВНУТРЕННИЙ угол 90 градусов\n";
        std::cout << "    2. Образует ВНЕШНИЙ угол 90 градусов\n";
        std::cout << "    3. Не образует угол 90 градусов (или открытый торец, будет торцевой профиль)\n";
        std::cout << "  Ваш выбор: ";
        int corner_choice;
        std::cin >> corner_choice;

        if (corner_choice == 1) {
            current_wall.SetNextCornerType(CornerType::Inner);
        }
        else if (corner_choice == 2) {
            current_wall.SetNextCornerType(CornerType::Outer);
        }
        else {
            current_wall.SetNextCornerType(CornerType::None);
            std::cout << "  [Инфо] Торец стены будет автоматически закрыт торцевым профилем.\n";
        }

        room_walls.push_back(current_wall);
        wall_counter++;
        std::cout << "==================================================\n\n";
    }

    // Обработка логики замыкания помещения
    if (!room_walls.empty()) {
        std::cout << "==================================================\n";
        std::cout << "Образуют ли ПЕРВАЯ и ПОСЛЕДНЯЯ стены угол 90 градусов (замкнутая комната)?\n";
        std::cout << "  1. Да, образуют ВНУТРЕННИЙ угол\n";
        std::cout << "  2. Да, образуют ВНЕШНИЙ угол\n";
        std::cout << "  3. Нет, это разомкнутый контур стен\n";
        std::cout << "Ваш выбор: ";
        int final_loop_choice;
        std::cin >> final_loop_choice;

        if (final_loop_choice == 1) {
            room_walls.back().SetNextCornerType(CornerType::Inner);
        }
        else if (final_loop_choice == 2) {
            room_walls.back().SetNextCornerType(CornerType::Outer);
        }
        else {
            // Если комната не замкнута, то:
            // 1. Правый торец последней стены остаётся открытым (CornerType::None уже стоит по умолчанию)
            // 2. Левый торец самой первой стены тоже открыт и потребует торцевого профиля.
            std::cout << "  [Инфо] Крайние торцы первой и последней стен будут закрыты торцевым профилем.\n";
        }

        // Запуск глобального расчета
        MaterialCalculator calculator;

        // Вектор-заглушка для внутренних швов панелей (наполним его на следующем шаге)
        std::vector<int> dummy_internal_seams;

        // 1. Считаем листовые панели
        std::vector<Material_Calculation_Result> panel_results = calculator.CalculateAllWalls(room_walls);

        // 2. Считаем по очереди каждый тип профиля из нашей базы данных
        std::vector<Material_Calculation_Result> profile_results;
        for (const auto& profile : profiles_db) {
            Material_Calculation_Result prof_res = calculator.CalculateProfile(profile, room_walls, dummy_internal_seams);
            profile_results.push_back(prof_res);
        }

        // ВЫВОД РЕЗУЛЬТАТОВ
        std::cout << "\n==================================================\n";
        std::cout << "             ИТОГОВЫЙ РАСЧЕТ РАСКРОЯ              \n";
        std::cout << "==================================================\n";

        std::cout << "\n[1] ЛИСТОВЫЕ МАТЕРИАЛЫ ДЛЯ ОТДЕЛКИ:\n";
        for (const auto& res : panel_results) {
            std::cout << "  Материал: " << res.material_name << " -> " << res.ones_used << " шт.\n";
        }

        std::cout << "\n[2] ПОГОНАЖНЫЕ ПРОФИЛИ И КРЕПЕЖ:\n";
        for (const auto& res : profile_results) {
            std::cout << "  Профиль: " << res.material_name << " -> " << res.ones_used << " шт. (хлыстов)\n";
        }
        std::cout << "==================================================\n";

    }
    else {
        std::cout << "Расчет отменен. Ни одной стены не было добавлено.\n";
    }

    return 0;
}

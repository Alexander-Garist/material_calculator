#include <iostream>
#include <string>
#include <vector>
#include <map>

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

// Стена класс, потому что содержит логику работы с ней
class Wall
{
private:
	int Wall_ID;
	int Wall_Width;		// Ширина стены
	int Wall_Height;	// Высота стены

	std::vector<FinishZone> Wall_Finish_Zones;	// Список зон отделки на конкретной стене

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
	const std::vector<FinishZone>& getFinishZones() const
	{
		return Wall_Finish_Zones;
	}

	// Геттер для получения ID стены
	int getID() const
	{
		return Wall_ID;
	}
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
};


// 4. КОНСОЛЬНЫЙ ИНТЕРФЕЙС (MAIN)

int main() {
    setlocale(LC_ALL, "ru");

    // Инициализируем несколько материалов по умолчанию
    std::vector<Material> materials_db = 
    {
        {"Панель (2800x1220, поворотный)", 1220, 2800, true},
        {"Рейка (3000x100, НЕ поворотная)", 100, 3000, false}
    };

    std::vector<Wall> room_walls;
    int wall_counter = 1;

    std::cout << "==================================================\n";
    std::cout << " Программа расчета листовых материалов для стен\n";
    std::cout << "==================================================\n\n";

    while (true) 
    {
        int wall_w, wall_h;
        std::cout << "--- Создание Стены [" << wall_counter << "] ---\n";
        std::cout << "Введите ширину стены в мм (или 0 для завершения ввода): ";
        std::cin >> wall_w;
        if (wall_w <= 0) break;

        std::cout << "Введите высоту стены в мм: ";
        std::cin >> wall_h;
        if (wall_h <= 0) 
        {
            std::cout << "[Ошибка] Высота должна быть больше 0. Попробуйте снова.\n";
            continue;
        }

        Wall current_wall(wall_counter, wall_w, wall_h);
        std::cout << "[Успех] Создана стена " << wall_w << "x" << wall_h << " мм.\n\n";

        // Цикл добавления зон отделки на текущую стену
        while (true) 
        {
            std::cout << "  Добавление зоны отделки для Стены №" << wall_counter << "\n";
            std::cout << "  Доступные материалы:\n";
            for (size_t i = 0; i < materials_db.size(); ++i) 
            {
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

            // Метод сам выполнит все геометрические проверки
            current_wall.AddFinishZone(z_x, z_y, z_w, z_h, selected_mat);
            std::cout << "\n";
        }

        room_walls.push_back(current_wall);
        wall_counter++;
        std::cout << "==================================================\n\n";
    }

    // Если пользователь ввёл хотя бы одну стену — запускаем расчёт
    if (!room_walls.empty()) {
        MaterialCalculator calculator;
        std::vector<Material_Calculation_Result> results = calculator.CalculateAllWalls(room_walls);

        std::cout << "\n==================================================\n";
        std::cout << "             ИТОГОВЫЙ РАСЧЕТ РАСКРОЯ              \n";
        std::cout << "==================================================\n";

        for (const auto& res : results) 
        {
            std::cout << "\nМатериал: " << res.material_name << "\n";
            std::cout << " -> Требуется целых единиц: " << res.ones_used << " шт.\n";
            std::cout << " -> Оставшиеся обрезки на складе (" << res.scraps.size() << " шт.):\n";

            if (res.scraps.empty()) 
            {
                std::cout << "    [Без отходов]\n";
            }
            else 
            {
                for (const auto& scrap : res.scraps) 
                {
                    std::cout << "    - " << scrap.width << " x " << scrap.height << " мм\n";
                }
            }
        }
        std::cout << "==================================================\n";
    } 
    else 
    {
        std::cout << "Расчет отменен. Ни одной стены не было добавлено.\n";
    }
    return 0;
}
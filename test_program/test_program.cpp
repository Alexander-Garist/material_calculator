#include <iostream>
#include <vector>
#include "Structures.h"
#include "Wall.h"
#include "MaterialCalculator.h"

int main() {
    setlocale(LC_ALL, "rus");

    std::vector<Material> materials_db = {
        {"Панель (2800x1220, поворотная)", 1220, 2800, true},
        {"Рейка (3000x100, НЕ поворотная)", 100, 3000, false}
    };

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
            int z_x = 0, z_y = 0, z_w = wall_w, z_h = wall_h;

            // Быстрое автозаполнение
            std::cout << "  Заполнить этим материалом ВСЮ стену целиком? (1 - Да, 0 - Вручную): ";
            int auto_fill;
            std::cin >> auto_fill;

            if (auto_fill == 0) {
                std::cout << "  Введите смещение X от левого края (мм): "; std::cin >> z_x;
                std::cout << "  Введите смещение Y от пола (мм): ";        std::cin >> z_y;
                std::cout << "  Введите ширину зоны (мм): ";               std::cin >> z_w;
                std::cout << "  Введите высоту зоны (мм): ";               std::cin >> z_h;
            }

            current_wall.AddFinishZone(z_x, z_y, z_w, z_h, selected_mat);
            std::cout << "\n";

            if (auto_fill == 1) break;
        }

        std::cout << "  --- Настройка стыка в конце Стены №" << wall_counter << " ---\n";
        std::cout << "  Как эта стена стыкуется со следующей?\n";
        std::cout << "    1. Образует ВНУТРЕННИЙ угол 90 градусов\n";
        std::cout << "    2. Образует ВНЕШНИЙ угол 90 градусов\n";
        std::cout << "    3. Не образует угол 90 градусов (будет торцевой профиль)\n";
        std::cout << "  Ваш выбор: ";
        int corner_choice;
        std::cin >> corner_choice;

        if (corner_choice == 1) current_wall.SetNextCornerType(CornerType::Inner);
        else if (corner_choice == 2) current_wall.SetNextCornerType(CornerType::Outer);
        else current_wall.SetNextCornerType(CornerType::None);

        room_walls.push_back(current_wall);
        wall_counter++;
        std::cout << "==================================================\n\n";
    }

    if (!room_walls.empty()) {
        std::cout << "==================================================\n";
        std::cout << "Образуют ли ПЕРВАЯ и ПОСЛЕДНЯЯ стены угол 90 градусов (замкнутая комната)?\n";
        std::cout << "  1. Да, ВНУТРЕННИЙ угол\n";
        std::cout << "  2. Да, ВНЕШНИЙ угол\n";
        std::cout << "  3. Нет, разомкнутый контур\n";
        std::cout << "Ваш выбор: ";
        int final_loop_choice;
        std::cin >> final_loop_choice;

        if (final_loop_choice == 1) room_walls.back().SetNextCornerType(CornerType::Inner);
        else if (final_loop_choice == 2) room_walls.back().SetNextCornerType(CornerType::Outer);

        MaterialCalculator calculator;
        std::vector<Material_Calculation_Result> panel_results = calculator.CalculateAllWalls(room_walls);

        // Собираем все внутренние швы со всех стен
        std::vector<int> accumulated_panel_seams;
        for (const auto& p_res : panel_results) {
            accumulated_panel_seams.insert(accumulated_panel_seams.end(), p_res.internal_seams.begin(), p_res.internal_seams.end());
        }

        std::vector<Material_Calculation_Result> profile_results;
        for (const auto& profile : profiles_db) {
            Material_Calculation_Result prof_res = calculator.CalculateProfile(profile, room_walls, accumulated_panel_seams);
            profile_results.push_back(prof_res);
        }

        std::cout << "\n==================================================\n";
        std::cout << "             ИТОГОВЫЙ РАСЧЕТ РАСКРОЯ              \n";
        std::cout << "==================================================\n";
        std::cout << "\n ЛИСТОВЫЕ МАТЕРИАЛЫ ДЛЯ ОТДЕЛКИ:\n";
        for (const auto& res : panel_results) {
            std::cout << "  Материал: " << res.material_name << " -> " << res.ones_used << " шт.\n";
        }
        std::cout << "\n ПОГОНАЖНЫЕ ПРОФИЛИ И КРЕПЕЖ:\n";
        for (const auto& res : profile_results) {
            std::cout << "  Профиль: " << res.material_name << " -> " << res.ones_used << " шт. (хлыстов)\n";
        }
        std::cout << "==================================================\n";
    }
    return 0;
}

#pragma once
#include "Structures.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

struct AppDatabase {
    std::vector<Material> materials;
    std::vector<ProfileMaterial> profiles;
};

class DataLoader {
private:
    static ProfileType StringToProfileType(const std::string& str) {
        if (str == "Торцевой" || str == "Edge") return ProfileType::Edge;
        if (str == "Соединительный" || str == "Connecting") return ProfileType::Connecting;
        if (str == "ВнутреннийУгол" || str == "InnerCorner") return ProfileType::InnerCorner;
        return ProfileType::OuterCorner;
    }

public:
    static AppDatabase LoadUnifiedDatabase(const std::string& filename) {
        AppDatabase db;
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "[Ошибка] Не удалось открыть единую базу данных: " << filename << "\n";
            return db;
        }

        std::string line;
        // Пропускаем первую строчку-заголовок Excel, если она есть
        std::getline(file, line);

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);

            std::string art, name, category, size_x, size_y, prof_type, rot_str, light_str;

            std::getline(ss, art, ';');
            std::getline(ss, name, ';');
            std::getline(ss, category, ';');
            std::getline(ss, size_x, ';');
            std::getline(ss, size_y, ';');
            std::getline(ss, prof_type, ';');
            std::getline(ss, rot_str, ';');
            std::getline(ss, light_str, ';');

            // Распределяем по контейнерам в зависимости от категории
            if (category == "Панель") {
                Material m{
                    art,
                    name,
                    size_x.empty() ? 0 : std::stoi(size_x),
                    size_y.empty() ? 0 : std::stoi(size_y),
                    (rot_str == "1")
                };
                db.materials.push_back(m);
            }
            else if (category == "Профиль") {
                ProfileMaterial p{
                    art,
                    name,
                    size_x.empty() ? 0 : std::stoi(size_x),
                    StringToProfileType(prof_type),
                    (light_str == "1")
                };
                db.profiles.push_back(p);
            }
        }
        return db;
    }
};

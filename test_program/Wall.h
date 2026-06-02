#pragma once
#include "Structures.h"
#include <vector>
#include <iostream>

class Wall {
private:
    int Wall_ID;
    int Wall_Width;
    int Wall_Height;
    std::vector<FinishZone> Wall_Finish_Zones;
    CornerType Next_Corner_Type = CornerType::None;

public:
    Wall(int id, int width, int height) : Wall_ID(id), Wall_Width(width), Wall_Height(height) {}

    void SetNextCornerType(CornerType type) { Next_Corner_Type = type; }
    CornerType getNextCornerType() const { return Next_Corner_Type; }
    int getWidth() const { return Wall_Width; }
    int getHeight() const { return Wall_Height; }
    int getID() const { return Wall_ID; }

    bool isOverlapping(const FinishZone& zone_1, const FinishZone& zone_2) const {
        return !((zone_1.offset_X + zone_1.width <= zone_2.offset_X) ||
            (zone_2.offset_X + zone_2.width <= zone_1.offset_X) ||
            (zone_1.offset_Y + zone_1.height <= zone_2.offset_Y) ||
            (zone_2.offset_Y + zone_2.height <= zone_1.offset_Y));
    }

    bool AddFinishZone(int offset_x, int offset_y, int width, int height, Material material) {
        FinishZone new_zone{ offset_x, offset_y, width, height, material };

        if (offset_x < 0 || offset_y < 0 || width <= 0 || height <= 0) {
            std::cout << "[Ошибка] Некорректные размеры или координаты зоны.\n";
            return false;
        }

        if (offset_x + width > Wall_Width || offset_y + height > Wall_Height) {
            std::cout << "[Ошибка] Зона выходит за физические границы стены!\n";
            return false;
        }

        for (const auto& existing_zone : Wall_Finish_Zones) {
            if (isOverlapping(new_zone, existing_zone)) {
                std::cout << "[Ошибка] Новая зона пересекается с уже существующей зоной!\n";
                return false;
            }
        }

        Wall_Finish_Zones.push_back(new_zone);
        std::cout << "[Успех] Зона отделки успешно добавлена на стену №" << Wall_ID << ".\n";
        return true;
    }

    const std::vector<FinishZone>& getFinishZones() const { return Wall_Finish_Zones; }
};

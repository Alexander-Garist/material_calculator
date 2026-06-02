#pragma once
#include <string>
#include <vector>

enum class ProfileType {
    Edge,         // Торцевой
    Connecting,   // Соединительный
    InnerCorner,  // Угловой внутренний
    OuterCorner   // Угловой внешний
};

enum class CornerType {
    None,
    Inner,
    Outer
};

struct Material {
    std::string name;
    int width;
    int height;
    bool can_rotate;

    bool operator<(const Material& other) const {
        return name < other.name;
    }
};

struct ProfileMaterial {
    std::string name;
    int length;
    ProfileType type;
    bool has_light;
};

struct Scrap {
    int width;
    int height;
};

struct FinishZone {
    int offset_X;
    int offset_Y;
    int width;
    int height;
    Material material;
};

struct Material_Calculation_Result {
    std::string material_name;
    int ones_used = 0;
    std::vector<Scrap> scraps;
    std::vector<int> internal_seams; // Автоматически собранные стыки панелей
};

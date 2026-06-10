#pragma once
#include <string>
#include <vector>

enum class ProfileType 
{
    Edge,         // Торцевой
    Connecting,   // Соединительный
    InnerCorner,  // Угловой внутренний
    OuterCorner   // Угловой внешний
};

enum class CornerType 
{
    None,
    Inner,
    Outer
};

struct Material 
{
    std::string article;    // Уникальный артикул
    std::string name;       // Название материала
    int width;              // Ширина материала
    int height;             // Высота материала
    bool can_rotate;        // Возможность поворота

    bool operator<(const Material& other) const 
    {
        return article < other.article;
    }
};

struct ProfileMaterial 
{
    std::string article;    // Уникальный артикул
    std::string name;       // Название профиля
    int length;             // Длина профиля
    ProfileType type;       // Тип профиля
    bool has_light;         // Наличие подсветки
};

struct Scrap 
{
    int width;
    int height;
};

struct FinishZone 
{
    int offset_X;
    int offset_Y;
    int width;
    int height;
    Material material;
};

struct Material_Calculation_Result 
{
    std::string material_name;
    int ones_used = 0;
    std::vector<Scrap> scraps;
    std::vector<int> internal_seams; // Автоматически собранные стыки панелей
};

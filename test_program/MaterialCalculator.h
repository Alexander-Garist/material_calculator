#pragma once
#include "Structures.h"
#include "Wall.h"
#include <map>
#include <algorithm>

class MaterialCalculator {
private:
    int Calculate1DMinPieces(const std::vector<int>& required_cuts, int profile_length) {
        if (required_cuts.empty()) return 0;

        std::vector<int> sorted_cuts = required_cuts;
        std::sort(sorted_cuts.begin(), sorted_cuts.end(), std::greater<int>());

        int pieces_used = 0;
        std::vector<int> scraps;
        int min_useful_length = profile_length / 3; // Обрезки профиля короче 1/3 хлыста — это мусор

        for (int cut : sorted_cuts) {
            while (cut > profile_length) {
                pieces_used++;
                cut -= profile_length;
            }

            bool found = false;
            for (size_t i = 0; i < scraps.size(); ++i) {
                if (scraps[i] >= cut) {
                    scraps[i] -= cut;
                    if (scraps[i] < min_useful_length) {
                        scraps.erase(scraps.begin() + i);
                    }
                    found = true;
                    break;
                }
            }

            if (!found) {
                pieces_used++;
                int rem = profile_length - cut;
                if (rem >= min_useful_length) {
                    scraps.push_back(rem);
                }
            }
        }
        return pieces_used;
    }

    Material_Calculation_Result CalculateSingleMaterial(const Material& material, const std::vector<FinishZone>& zones) {
        Material_Calculation_Result result;
        result.material_name = material.name;

        if (zones.empty()) return result;

        std::vector<Scrap> current_scraps;

        for (const auto& zone : zones) {
            for (int y = 0; y < zone.height; y += material.height) {
                int current_row_H = std::min(material.height, zone.height - y);

                for (int x = 0; x < zone.width; ) {
                    int current_needed_W = zone.width - x;
                    bool found_scrap = false;
                    size_t scrap_index = 0;

                    for (size_t i = 0; i < current_scraps.size(); ++i) {
                        if (current_scraps[i].width >= current_needed_W && current_scraps[i].height >= current_row_H) {
                            found_scrap = true;
                            scrap_index = i;
                            break;
                        }
                        if (material.can_rotate && current_scraps[i].height >= current_needed_W && current_scraps[i].width >= current_row_H) {
                            std::swap(current_scraps[i].width, current_scraps[i].height);
                            found_scrap = true;
                            scrap_index = i;
                            break;
                        }
                    }

                    int current_piece_W = 0;
                    int current_piece_H = 0;

                    if (found_scrap) {
                        current_piece_W = current_scraps[scrap_index].width;
                        if (current_scraps[scrap_index].height > current_row_H) {
                            current_scraps.push_back({ current_piece_W, current_scraps[scrap_index].height - current_row_H });
                            current_piece_H = current_row_H;
                        }
                        else {
                            current_piece_H = current_scraps[scrap_index].height;
                        }
                        current_scraps.erase(current_scraps.begin() + scrap_index);
                    }
                    else {
                        result.ones_used++;
                        current_piece_W = material.width;
                        current_piece_H = material.height;
                    }

                    int used_W = std::min(current_piece_W, current_needed_W);
                    int used_H = std::min(current_piece_H, current_row_H);

                    if (current_piece_W > used_W) {
                        current_scraps.push_back({ current_piece_W - used_W, current_piece_H });
                    }
                    if (current_piece_H > used_H) {
                        current_scraps.push_back({ used_W, current_piece_H - used_H });
                    }

                    x += used_W;
                    // ФИКС: Если мы уложили панель, и зона справа НЕ закончилась,
                    // значит здесь физически образуется вертикальный внутренний шов.
                    if (x < zone.width) {
                        result.internal_seams.push_back(used_H);
                    }
                }
            }
        }

        result.scraps = current_scraps;
        return result;
    }

public:
    std::vector<Material_Calculation_Result> CalculateAllWalls(const std::vector<Wall>& room_walls) {
        std::map<Material, std::vector<FinishZone>> grouped_zones;

        for (const auto& wall : room_walls) {
            for (const auto& zone : wall.getFinishZones()) {
                grouped_zones[zone.material].push_back(zone);
            }
        }

        std::vector<Material_Calculation_Result> final_results;
        for (const auto& kv : grouped_zones) {
            Material_Calculation_Result mat_res = CalculateSingleMaterial(kv.first, kv.second);
            final_results.push_back(mat_res);
        }
        return final_results;
    }

    Material_Calculation_Result CalculateProfile(const ProfileMaterial& profile,
        const std::vector<Wall>& room_walls,
        const std::vector<int>& all_panel_seams) {
        Material_Calculation_Result result;
        result.material_name = profile.name + (profile.has_light ? " (с подсветкой)" : "");

        std::vector<int> required_cuts;

        if (profile.type == ProfileType::Edge) {
            // Горизонтальный погонаж (пол и потолок для каждой зоны)
            for (const auto& wall : room_walls) {
                for (const auto& zone : wall.getFinishZones()) {
                    required_cuts.push_back(zone.width);
                    required_cuts.push_back(zone.width);
                }
            }
            // Вертикальные незакрытые торцы (края разомкнутых стен)
            if (!room_walls.empty() && room_walls.back().getNextCornerType() == CornerType::None) {
                required_cuts.push_back(room_walls.front().getHeight());
            }
            for (const auto& wall : room_walls) {
                if (wall.getNextCornerType() == CornerType::None) {
                    required_cuts.push_back(wall.getHeight());
                }
            }
        }
        else if (profile.type == ProfileType::Connecting) {
            required_cuts = all_panel_seams; // Сюда попадают швы из CalculateSingleMaterial
        }
        else if (profile.type == ProfileType::InnerCorner) {
            for (const auto& wall : room_walls) {
                if (wall.getNextCornerType() == CornerType::Inner) {
                    required_cuts.push_back(wall.getHeight());
                }
            }
        }
        else if (profile.type == ProfileType::OuterCorner) {
            for (const auto& wall : room_walls) {
                if (wall.getNextCornerType() == CornerType::Outer) {
                    required_cuts.push_back(wall.getHeight());
                }
            }
        }

        result.ones_used = Calculate1DMinPieces(required_cuts, profile.length);
        return result;
    }
};

#include "sr2ap/Collectibles.hpp"

namespace sr2ap {
    namespace {
        const std::vector<CdDefinition> kCdDefinitions{
            {0xDCA6FB9B, "rebadeaux_1"},
            {0x45AFAA21, "harrowgate_1"},
            {0xE38D99D3, "rebadeaux_2"},
            {0x948AA945, "shivington_1"},
            {0xD510B7B0, "prawn_court_1"},
            {0x93E76D5C, "stoughton_1"},
            {0x7DE90C70, "stoughton_2"},
            {0xE4E05DCA, "southern_cross_1"},
            {0xED5611E1, "huntersfield_1"},
            {0x1985C974, "wardill_airport_1"},
            {0x893AD4E5, "wardill_airport_2"},
            {0x7A84C869, "ezpata_1"},
            {0xA2178726, "athos_bay_1"},
            {0x32A89AB7, "mission_beach_1"},
            {0x0D83F8FF, "little_shanghai_1"},
            {0x9A512177, "stilwater_university_1"},
            {0xFA96A892, "stilwater_university_2"},
            {0x0AEE3CE6, "frat_row_1"},
            {0xE9FD5D00, "stilwater_nuclear_1"},
            {0x9EFA6D96, "stilwater_nuclear_2"},
            {0xFE3DE473, "stilwater_nuclear_3"},
            {0x6E82F9E2, "stilwater_nuclear_4"},
            {0x808C98CE, "hangman_s_wharf_1"},
            {0xF0E66C41, "hangman_s_wharf_2"},
            {0x69EF3DFB, "stilwater_penitentiary_1"},
            {0xF78BA858, "stilwater_penitentiary_2"},
            {0x1EE80D6D, "stilwater_penitentiary_3"},
            {0x70F40CBA, "no_district_1"},
            {0x35C55EAE, "brighton_1"},
            {0xABA1CB0D, "brighton_2"},
            {0x07F33C2C, "brighton_3"},
            {0x42C26E38, "adept_way_1"},
            {0xDBCB3F82, "amberbrook_1"},
            {0xACCC0F14, "amberbrook_2"},
            {0xCC0B86F1, "amberbrook_3"},
            {0xBB0CB667, "centennial_beach_1"},
            {0x2BB3ABF6, "centennial_beach_2"},
            {0xC5BDCADA, "stilwater_boardwalk_1"},
            {0x5CB49B60, "stilwater_boardwalk_2"},
            {0x9021E5A4, "misty_lane_1"},
            {0x009EF835, "new_hennequet_1"},
            {0x7799C8A3, "quinbecca_1"},
            {0x9997A98F, "elysian_fields_1"},
            {0xEE909919, "misty_lane_2"},
            {0xE726D532, "misty_lane_3"},
            {0xC2D00EC3, "misty_lane_4"},
            {0xB5D73E55, "tidal_spring_1"},
            {0x5BD95F79, "tidal_spring_2"},
            {0x2CDE6FEF, "nob_hill_1"},
            {0xB2BAFA4C, "stilwater_boardwalk_3"},
        };
    }

    const std::vector<CdDefinition>& GetCdDefinitions() {
        return kCdDefinitions;
    }

    const char* FindCdDistrictKey(const std::uint32_t id) {
        for (const auto& definition : kCdDefinitions) {
            if (definition.id == id) {
                return definition.districtKey;
            }
        }
        return nullptr;
    }
}  // namespace sr2ap

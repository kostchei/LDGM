
#pragma once

namespace LDMChronoVehicle
{
    // System Component TypeIds
    inline constexpr const char* LDMChronoVehicleSystemComponentTypeId = "{692B6C2D-C8A1-4CDB-8492-BC8E52AA2D16}";
    inline constexpr const char* LDMChronoVehicleEditorSystemComponentTypeId = "{AAB28B95-695D-4AC5-89BB-A1B3BC0B9DED}";

    // Module derived classes TypeIds
    inline constexpr const char* LDMChronoVehicleModuleInterfaceTypeId = "{91B78A59-45BC-4905-A027-F1125B6C661F}";
    inline constexpr const char* LDMChronoVehicleModuleTypeId = "{1DEF6F6F-7825-479A-84D3-379A039E4E39}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* LDMChronoVehicleEditorModuleTypeId = LDMChronoVehicleModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* LDMChronoVehicleRequestsTypeId = "{3D6EEE5F-6B74-4011-BEA7-2C2634D101C6}";

    // Entity Component TypeIds
    inline constexpr const char* VehicleProxyComponentTypeId = "{7E3A2A41-58D0-4A5E-9B0C-2F6D1C6B8A57}";
} // namespace LDMChronoVehicle

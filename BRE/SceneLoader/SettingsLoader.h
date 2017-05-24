#pragma once

namespace YAML {
class Node;
}

namespace BRE {

///
/// @brief Responsible to load from scene file the settings 
///
class SettingsLoader {
public:
    SettingsLoader() = default;
    SettingsLoader(const SettingsLoader&) = delete;
    const SettingsLoader& operator=(const SettingsLoader&) = delete;
    SettingsLoader(SettingsLoader&&) = delete;
    SettingsLoader& operator=(SettingsLoader&&) = delete;

    ///
    /// @brief Load settings
    /// @param rootNode Scene YAML file root node
    ///
    void LoadSettings(const YAML::Node& rootNode) noexcept;
};
}
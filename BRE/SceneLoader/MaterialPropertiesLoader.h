#pragma once

#include <string>
#include <unordered_map>

#include <ShaderUtils/MaterialProperties.h>

namespace YAML {
class Node;
}

namespace BRE {
///
/// @brief Responsible to load from scene file the material properties
///
class MaterialPropertiesLoader {
public:
    MaterialPropertiesLoader()
    {}
    MaterialPropertiesLoader(const MaterialPropertiesLoader&) = delete;
    const MaterialPropertiesLoader& operator=(const MaterialPropertiesLoader&) = delete;
    MaterialPropertiesLoader(MaterialPropertiesLoader&&) = delete;
    MaterialPropertiesLoader& operator=(MaterialPropertiesLoader&&) = delete;

    ///
    /// @brief Load material properties
    /// @param rootNode Scene YAML file root node
    ///
    void LoadMaterialsProperties(const YAML::Node& rootNode) noexcept;

    ///
    /// @brief Get material properties
    /// @param name Material properties name
    /// @return Material properties
    ///
    const MaterialProperties& GetMaterialProperties(const std::string& name) const noexcept;

    ///
    /// @brief Get default material properties
    ///
    /// This technique is used when 'material properties' is not specified
    /// in a drawable object.
    ///
    /// @return Default MaterialProperties
    ///
    const MaterialProperties& GetDefaultMaterialProperties() const noexcept
    {
        return mDefaultMaterialProperties;
    }

private:
    std::unordered_map<std::string, MaterialProperties> mMaterialPropertiesByName;

    // Default material properties to use when 'material properties' is not defined
    // in a drawable object.
    MaterialProperties mDefaultMaterialProperties{ 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
};
}
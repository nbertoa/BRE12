#include "SettingsLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <ApplicationSettings\ApplicationSettings.h>
#include <EnvironmentLightPass\EnvironmentLightSettings.h>
#include <GeometryPass\GeometrySettings.h>
#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

namespace BRE {
void
SettingsLoader::LoadSettings(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "settings" node. It is a single sequence with a map and its sintax is:
    // camera:
    //   - screen width: N
    //     screen height: M
    //     CPU processors: K
    const YAML::Node settingsNode = rootNode["settings"];
    if (settingsNode.IsDefined() == false) {
        return;
    }

    BRE_CHECK_MSG(settingsNode.IsSequence(), L"'settings' node must be a sequence");

    BRE_CHECK_MSG(settingsNode.begin() != settingsNode.end(), L"'settings' node is empty");
    const YAML::Node settingsMap = *settingsNode.begin();
    BRE_CHECK_MSG(settingsMap.IsMap(), L"'settings' node first sequence is not a map");

    std::string propertyName;
    YAML::const_iterator mapIt = settingsMap.begin();
    while (mapIt != settingsMap.end()) {
        propertyName = mapIt->first.as<std::string>();

        if (propertyName == "screen height") {
            YamlUtils::GetScalar(mapIt->second, 
                                 ApplicationSettings::sWindowHeight);
            ApplicationSettings::sScissorRect.bottom = 
                static_cast<LONG>(ApplicationSettings::sWindowHeight);
            ApplicationSettings::sScreenViewport.Height = 
                static_cast<float>(ApplicationSettings::sWindowHeight);
        } else if (propertyName == "screen width") {
            YamlUtils::GetScalar(mapIt->second, 
                                 ApplicationSettings::sWindowWidth);
            ApplicationSettings::sScissorRect.right = 
                static_cast<LONG>(ApplicationSettings::sWindowWidth);
            ApplicationSettings::sScreenViewport.Width = 
                static_cast<float>(ApplicationSettings::sWindowWidth);
        } else if (propertyName == "CPU processors") {
            YamlUtils::GetScalar(mapIt->second, 
                                 ApplicationSettings::sCpuProcessorCount);
        } else if (propertyName == "near plane z") {
            YamlUtils::GetScalar(mapIt->second,
                                 ApplicationSettings::sNearPlaneZ);
        } else if (propertyName == "far plane z") {
            YamlUtils::GetScalar(mapIt->second, 
                                 ApplicationSettings::sFarPlaneZ);
        } else if (propertyName == "vertical field of view") {
            YamlUtils::GetScalar(mapIt->second, 
                                 ApplicationSettings::sVerticalFieldOfView);
        } else if (propertyName == "fullscreen") {
            std::uint32_t isFullscreen;
            YamlUtils::GetScalar(mapIt->second, 
                                 isFullscreen);
            ApplicationSettings::sIsFullscreenWindow = isFullscreen > 0U;
        } else if (propertyName == "ambient occlusion sample kernel size") {
            YamlUtils::GetScalar(mapIt->second,
                                 EnvironmentLightSettings::sSampleKernelSize);
        } else if (propertyName == "ambient occlusion noise texture dimension") {
            YamlUtils::GetScalar(mapIt->second,
                                 EnvironmentLightSettings::sNoiseTextureDimension);
        } else if (propertyName == "ambient occlusion radius") {
            YamlUtils::GetScalar(mapIt->second,
                                 EnvironmentLightSettings::sOcclusionRadius);
        } else if (propertyName == "ambient occlusion power") {
            YamlUtils::GetScalar(mapIt->second,
                                 EnvironmentLightSettings::sSsaoPower);
        } else if (propertyName == "height mapping min tessellation distance") {
            YamlUtils::GetScalar(mapIt->second,
                                 GeometrySettings::sMinTessellationDistance);
        } else if (propertyName == "height mapping max tessellation distance") {
            YamlUtils::GetScalar(mapIt->second,
                                 GeometrySettings::sMaxTessellationDistance);
        } else if (propertyName == "height mapping min tessellation factor") {
            YamlUtils::GetScalar(mapIt->second,
                                 GeometrySettings::sMinTessellationFactor);
        } else if (propertyName == "height mapping max tessellation factor") {
            YamlUtils::GetScalar(mapIt->second,
                                 GeometrySettings::sMaxTessellationFactor);
        } else if (propertyName == "height mapping height scale") {
            YamlUtils::GetScalar(mapIt->second,
                                 GeometrySettings::sHeightScale);
        } else {
            // To avoid warning about 'conditional expression is constant'. This is the same than false
            const std::wstring errorMsg =
                L"Unknown settings field: " + StringUtils::AnsiToWideString(propertyName);
            BRE_CHECK_MSG(&propertyName == nullptr, errorMsg.c_str());
        }

        ++mapIt;
    }
}
}
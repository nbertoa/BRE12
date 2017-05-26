#include "CameraLoader.h"

#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <SceneLoader\YamlUtils.h>
#include <Utils/DebugUtils.h>

using namespace DirectX;

namespace BRE {
void
CameraLoader::LoadCamera(const YAML::Node& rootNode) noexcept
{
    BRE_ASSERT(rootNode.IsDefined());

    // Get the "camera" node. It is a single sequence with a map and its sintax is:
    // camera:
    //   - position: [0.0, 0.0, 0.0]
    //     look vector: [0.0, 0.0, 1.0]
    //     up vector: [0.0, 1.0, 0.0]
    //     vertical field of view: scalar
    //     near plane z: scalar
    //     far plane z: scalar
    const YAML::Node cameraNode = rootNode["camera"];
    if (cameraNode.IsDefined() == false) {
        return;
    }

    BRE_CHECK_MSG(cameraNode.IsSequence(), L"'camera' node must be a sequence");

    BRE_CHECK_MSG(cameraNode.begin() != cameraNode.end(), L"'camera' node is empty");
    const YAML::Node cameraMap = *cameraNode.begin();
    BRE_CHECK_MSG(cameraMap.IsMap(), L"'camera' node first sequence is not a map");

    // Get data to set camera
    float position[3]{ 0.0f, 0.0f, 0.0f };
    float lookVector[3]{ 0.0f, 0.0f, 1.0f };
    float upVector[3]{ 0.0f, 1.0f, 0.0f };
    std::string propertyName;
    YAML::const_iterator mapIt = cameraMap.begin();
    while (mapIt != cameraMap.end()) {
        propertyName = mapIt->first.as<std::string>();

        if (propertyName == "position") {
            YamlUtils::GetSequence(mapIt->second, position, 3U);
        } else if (propertyName == "look vector") {
            YamlUtils::GetSequence(mapIt->second, lookVector, 3U);
        } else if (propertyName == "up vector") {
            YamlUtils::GetSequence(mapIt->second, upVector, 3U);
        } else {
            // To avoid warning about 'conditional expression is constant'. This is the same than false
            const std::wstring errorMsg =
                L"Unknown camera field: " + StringUtils::AnsiToWideString(propertyName);
            BRE_CHECK_MSG(&propertyName == nullptr, errorMsg.c_str());
        }

        ++mapIt;
    }

    mCamera.SetPosition(XMFLOAT3(position[0U], position[1U], position[2U]));
    mCamera.SetLookAndUpVectors(XMFLOAT3(lookVector[0U], lookVector[1U], lookVector[2U]),
                                XMFLOAT3(upVector[0U], upVector[1U], upVector[2U]));
}
}
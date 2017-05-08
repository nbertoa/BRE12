#pragma once

#include <string>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <Utils\DebugUtils.h>

namespace BRE {
class YamlUtils {
public:
    YamlUtils() = delete;
    ~YamlUtils() = delete;
    YamlUtils(const YamlUtils&) = delete;
    const YamlUtils& operator=(const YamlUtils&) = delete;
    YamlUtils(YamlUtils&&) = delete;
    YamlUtils& operator=(YamlUtils&&) = delete;

    static bool IsDefined(const YAML::Node& node,
                          const char* key);

    template<typename T>
    static T GetScalar(const YAML::Node& node,
                       const char* key)
    {
        BRE_ASSERT(key != nullptr);
        YAML::Node attr = node[key];
        BRE_ASSERT(attr.IsDefined());
        BRE_ASSERT(attr.IsScalar());
        return std::to_string(attr.as<std::string>());
    }

    template<>
    static std::string GetScalar(const YAML::Node& node,
                                 const char* key)
    {
        BRE_ASSERT(key != nullptr);
        YAML::Node attr = node[key];
        BRE_ASSERT(attr.IsDefined());
        BRE_ASSERT(attr.IsScalar());
        return attr.as<std::string>();
    }

    template<typename T>
    static void GetSequence(const YAML::Node& node,
                            T* const sequenceOutput,
                            const size_t numElems)
    {
        BRE_ASSERT(sequenceOutput != nullptr);
        BRE_ASSERT(node.IsDefined());
        BRE_ASSERT(node.IsSequence());
        size_t currentNumElems = 0;
        for (const YAML::Node& seqNode : node) {
            BRE_ASSERT(seqNode.IsScalar());
            BRE_ASSERT(currentNumElems < numElems);
            sequenceOutput[currentNumElems] = seqNode.as<T>();
            ++currentNumElems;
        }
        BRE_ASSERT(currentNumElems == numElems);
    }
};

}


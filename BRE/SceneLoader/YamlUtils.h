#pragma once

#include <string>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <Utils\DebugUtils.h>

namespace BRE {
///
/// @brief Utilities to handle YAML format 
///
class YamlUtils {
public:
    YamlUtils() = delete;
    ~YamlUtils() = delete;
    YamlUtils(const YamlUtils&) = delete;
    const YamlUtils& operator=(const YamlUtils&) = delete;
    YamlUtils(YamlUtils&&) = delete;
    YamlUtils& operator=(YamlUtils&&) = delete;

    ///
    /// @brief Get scalar 
    /// @param node YAML node
    /// @param scalar Output scalar
    ///
    template<typename T>
    static void GetScalar(const YAML::Node& node, T& scalar) noexcept
    {
        BRE_ASSERT(node.IsDefined());
        BRE_ASSERT(node.IsScalar());
        scalar = node.as<T>();
    }

    ///
    /// @brief Get sequence
    /// @param node YAML node
    /// @param sequenceOutput Output sequence
    /// @param numElems Number of elements in the sequence
    ///
    template<typename T>
    static void GetSequence(const YAML::Node& node,
                            T* const sequenceOutput,
                            const size_t numElems) noexcept
    {
        BRE_ASSERT(sequenceOutput != nullptr);
        BRE_ASSERT(node.IsDefined());
        BRE_ASSERT(node.IsSequence());
        size_t currentNumElems = 0UL;
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
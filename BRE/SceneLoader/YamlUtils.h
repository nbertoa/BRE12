#pragma once

#include <string>
#pragma warning( push )
#pragma warning( disable : 4127)
#include <yaml-cpp/yaml.h>
#pragma warning( pop ) 

#include <Utils\DebugUtils.h>

class YamlUtils {
public:
	YamlUtils() = delete;
	~YamlUtils() = delete;
	YamlUtils(const YamlUtils&) = delete;
	const YamlUtils& operator=(const YamlUtils&) = delete;
	YamlUtils(YamlUtils&&) = delete;
	YamlUtils& operator=(YamlUtils&&) = delete;

	static bool IsDefined(const YAML::Node& node, const char* key);

	template<typename T>
	static T GetScalar(const YAML::Node& node, const char* key) {
		ASSERT(key != nullptr);
		YAML::Node attr = node[key];
		ASSERT(attr.IsDefined());
		ASSERT(attr.IsScalar());
		return std::to_string(attr.as<std::string>());
	}

	template<>
	static std::string GetScalar(const YAML::Node& node, const char* key) {
		ASSERT(key != nullptr);
		YAML::Node attr = node[key];
		ASSERT(attr.IsDefined());
		ASSERT(attr.IsScalar());
		return attr.as<std::string>();
	}

	template<typename T>
	static void GetSequence(const YAML::Node& node, T* const sequenceOutput, const size_t numElems) {
		ASSERT(sequenceOutput != nullptr);
		ASSERT(node.IsDefined());
		ASSERT(node.IsSequence());
		size_t currentNumElems = 0;
		for (const YAML::Node& seqNode : node) {
			ASSERT(seqNode.IsScalar());
			ASSERT(currentNumElems < numElems);
			sequenceOutput[currentNumElems] = seqNode.as<T>();
			++currentNumElems;
		}
		ASSERT(currentNumElems == numElems);
	}
};

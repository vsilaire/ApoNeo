#pragma once

#include "FlameGenome.hpp"
#include <string>
#include <vector>

namespace ApoNeo::Core {

class FlameXml {
public:
    /// @brief Parse all flames contained in an XML string or .flame file content
    static std::vector<FlameGenome> parse_flames(const std::string& xml_content, std::string* error_out = nullptr);

    /// @brief Parse single flame from XML string
    static bool parse_single_flame(const std::string& xml_content, FlameGenome& out_genome, std::string* error_out = nullptr);

    /// @brief Read .flame file from disk
    static std::vector<FlameGenome> load_from_file(const std::string& filepath, std::string* error_out = nullptr);

    /// @brief Serialize a single flame or list of flames into standard .flame XML string
    static std::string serialize_flame(const FlameGenome& genome);
    static std::string serialize_flames(const std::vector<FlameGenome>& genomes);

    /// @brief Save flame to disk
    static bool save_to_file(const std::string& filepath, const FlameGenome& genome, std::string* error_out = nullptr);
    static bool save_flames_to_file(const std::string& filepath, const std::vector<FlameGenome>& genomes, std::string* error_out = nullptr);
};

} // namespace ApoNeo::Core

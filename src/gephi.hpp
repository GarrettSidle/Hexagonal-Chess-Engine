#pragma once

#include "search.hpp"
#include <string>

namespace hexchess {
namespace gephi {

// Set the base directory for GEXF exports (default: current directory).
// Call with executable directory to ensure gephi_exports is created next to the engine.
void set_export_base_dir(const std::string& dir);

// Export the search tree to a GEXF file for Gephi visualization.
// Creates parent directory if needed. Nodes have score, depth, move attributes; edges have move labels.
void export_tree(const search::Node& root, const std::string& path);

}  // namespace gephi
}  // namespace hexchess

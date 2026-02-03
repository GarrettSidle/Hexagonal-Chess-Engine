#pragma once

#include "search.hpp"
#include <string>

namespace hexchess {
namespace gephi {

// Export the search tree to a GEXF file for Gephi visualization.
// Creates parent directory if needed. Nodes have score, depth, move attributes; edges have move labels.
void export_tree(const search::Node& root, const std::string& path);

}  // namespace gephi
}  // namespace hexchess

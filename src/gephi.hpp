#pragma once

#include "search.hpp"
#include <string>

namespace hexchess {
namespace gephi {

// base dir for gexf exports (default cwd). set to exe dir so gephi_exports ends up next to engine
void set_export_base_dir(const std::string& dir);

// dump search tree to gexf for gephi. nodes: score,depth,move; edges: move label
void export_tree(const search::Node& root, const std::string& path);

}  // namespace gephi
}  // namespace hexchess

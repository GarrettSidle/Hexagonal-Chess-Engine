#include "gephi.hpp"
#include "protocol.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace hexchess {
namespace gephi {

using namespace board;

static std::string g_export_base_dir;

void set_export_base_dir(const std::string& dir) {
  g_export_base_dir = dir;
}

static std::string escape_xml(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&apos;"; break;
      default: out += c; break;
    }
  }
  return out;
}

static std::string move_label(const State& parent_state, const Move& m) {
  auto piece = parent_state.at(m.from_col, m.from_row);
  auto captured = parent_state.at(m.to_col, m.to_row);
  char pt = piece ? piece->type : 'P';
  std::optional<char> cap_type = captured ? std::optional<char>(captured->type) : std::nullopt;
  if (m.en_passant && piece)
    return protocol::format_move_ep(m, piece->white);
  return protocol::format_move_long(m, pt, cap_type);
}

static void write_node(std::ostringstream& nodes_out,
    const std::string& id, const std::string& label, int score, int depth, const std::string& move_str) {
  nodes_out << "<node id=\"" << escape_xml(id) << "\" label=\"" << escape_xml(label) << "\">\n";
  nodes_out << "  <attvalues>";
  nodes_out << "<attvalue for=\"score\" value=\"" << score << "\"/>";
  nodes_out << "<attvalue for=\"depth\" value=\"" << depth << "\"/>";
  nodes_out << "<attvalue for=\"move\" value=\"" << escape_xml(move_str) << "\"/>";
  nodes_out << "</attvalues>\n</node>\n";
}

static void walk_tree(const search::Node& node, const State* parent_state, const Move* incoming_move,
    int depth, int& next_id, int& next_edge_id,
    std::ostringstream& nodes_out, std::ostringstream& edges_out) {
  std::string node_id = "n" + std::to_string(next_id);
  int my_id = next_id;
  next_id++;

  std::string label = (depth == 0) ? "root" : node_id;
  std::string move_str = incoming_move
      ? move_label(*parent_state, *incoming_move)
      : "";

  write_node(nodes_out, node_id, label, node.best_score, depth, move_str);

  for (const auto& [m, child] : node.children) {
    if (!child) continue;
    std::string target_id = "n" + std::to_string(next_id);
    std::string edge_label = move_label(node.state, m);
    edges_out << "<edge id=\"e" << next_edge_id++ << "\" source=\"" << node_id
              << "\" target=\"" << target_id << "\" label=\"" << escape_xml(edge_label) << "\"/>\n";
    walk_tree(*child, &node.state, &m, depth + 1, next_id, next_edge_id, nodes_out, edges_out);
  }
}

static bool try_write(const std::filesystem::path& p, const std::ostringstream& nodes_ss, const std::ostringstream& edges_ss) {
  if (p.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) return false;
  }
  std::ofstream f(p);
  if (!f) return false;
  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  f << "<gexf xmlns=\"http://www.gexf.net/1.3\" version=\"1.3\">\n";
  f << "  <graph mode=\"static\" defaultedgetype=\"directed\">\n";
  f << "    <attributes class=\"node\">\n";
  f << "      <attribute id=\"score\" title=\"Score\" type=\"integer\"/>\n";
  f << "      <attribute id=\"depth\" title=\"Depth\" type=\"integer\"/>\n";
  f << "      <attribute id=\"move\" title=\"Move\" type=\"string\"/>\n";
  f << "    </attributes>\n";
  f << "    <nodes>\n" << nodes_ss.str() << "    </nodes>\n";
  f << "    <edges>\n" << edges_ss.str() << "    </edges>\n";
  f << "  </graph>\n</gexf>\n";
  return true;
}

void export_tree(const search::Node& root, const std::string& path) {
  std::ostringstream nodes_ss;
  std::ostringstream edges_ss;
  int next_id = 0;
  int next_edge_id = 0;

  walk_tree(root, nullptr, nullptr, 0, next_id, next_edge_id, nodes_ss, edges_ss);

  std::filesystem::path p = g_export_base_dir.empty()
      ? std::filesystem::path(path)
      : (std::filesystem::path(g_export_base_dir) / path);

  if (try_write(p, nodes_ss, edges_ss))
    return;

  std::filesystem::path fallback = std::filesystem::current_path() / path;
  if (try_write(fallback, nodes_ss, edges_ss))
    return;
}

}  // namespace gephi
}  // namespace hexchess

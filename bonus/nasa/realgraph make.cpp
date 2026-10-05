#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct Point {
    double x;
    double y;
    double z;
    int zone;
    int local;
};

struct Zone {
    std::string name;
    int nodes = 0;
    int elements = 0;
    std::string type;
};

struct Edge {
    uint32_t u;
    uint32_t v;

    bool operator<(const Edge& other) const {
        if (u != other.u) {
            return u < other.u;
        }
        return v < other.v;
    }

    bool operator==(const Edge& other) const {
        return u == other.u && v == other.v;
    }
};

class PipeBuf : public std::streambuf {
    FILE* pipe_;
    char buffer_[64 * 1024];

public:
    explicit PipeBuf(FILE* pipe) : pipe_(pipe) {
        setg(buffer_, buffer_, buffer_);
    }

protected:
    int_type underflow() override {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        const std::size_t n =
            std::fread(buffer_, 1, sizeof(buffer_), pipe_);

        if (n == 0) {
            return traits_type::eof();
        }

        setg(buffer_, buffer_, buffer_ + n);
        return traits_type::to_int_type(*gptr());
    }
};

static bool is_volume(const std::string& name) {
    return name == "TETRAHEDRA" ||
           name == "PYRAMIDS" ||
           name == "PRISMS";
}

static bool is_boundary(const std::string& name) {
    return name.size() >= 2 &&
           name[0] == 'B' &&
           name[1] == '_';
}

static std::string zone_name(const std::string& line) {
    const auto first = line.find('"');
    const auto second = line.find('"', first + 1);

    if (first == std::string::npos ||
        second == std::string::npos) {
        throw std::runtime_error("Malformed ZONE header.");
    }

    return line.substr(first + 1, second - first - 1);
}

static int parse_int_after(const std::string& line,
                           const std::string& key) {
    const auto pos = line.find(key);

    if (pos == std::string::npos) {
        throw std::runtime_error("Missing " + key);
    }

    std::size_t begin = pos + key.size();

    while (begin < line.size() &&
           std::isspace(static_cast<unsigned char>(line[begin]))) {
        ++begin;
    }

    std::size_t end = begin;

    while (end < line.size() &&
           std::isdigit(static_cast<unsigned char>(line[end]))) {
        ++end;
    }

    return std::stoi(line.substr(begin, end - begin));
}

static std::string parse_string_after(const std::string& line,
                                      const std::string& key) {
    const auto pos = line.find(key);

    if (pos == std::string::npos) {
        return {};
    }

    std::size_t begin = pos + key.size();

    while (begin < line.size() &&
           std::isspace(static_cast<unsigned char>(line[begin]))) {
        ++begin;
    }

    std::size_t end = begin;

    while (end < line.size() &&
           !std::isspace(static_cast<unsigned char>(line[end])) &&
           line[end] != ',') {
        ++end;
    }

    return line.substr(begin, end - begin);
}

static Zone read_zone_header(std::istream& in,
                             const std::string& first_line) {
    Zone z;
    z.name = zone_name(first_line);

    std::string line;

    while (std::getline(in, line)) {
        if (line.find("Nodes=") != std::string::npos) {
            z.nodes = parse_int_after(line, "Nodes=");
            z.elements = parse_int_after(line, "Elements=");
            z.type = parse_string_after(line, "ZONETYPE=");
        }

        if (line.find("DT=") != std::string::npos) {
            break;
        }
    }

    if (z.nodes <= 0 || z.elements <= 0 || z.type.empty()) {
        throw std::runtime_error(
            "Invalid metadata for zone " + z.name);
    }

    return z;
}

static void consume_coordinates(std::istream& in,
                                int n,
                                std::vector<Point>* points,
                                int zone_id) {
    std::vector<double> x(n);
    std::vector<double> y(n);
    std::vector<double> z(n);

    for (int i = 0; i < n; ++i) {
        if (!(in >> x[i])) {
            throw std::runtime_error("EOF in X coordinates.");
        }
    }

    for (int i = 0; i < n; ++i) {
        if (!(in >> y[i])) {
            throw std::runtime_error("EOF in Y coordinates.");
        }
    }

    for (int i = 0; i < n; ++i) {
        if (!(in >> z[i])) {
            throw std::runtime_error("EOF in Z coordinates.");
        }
    }

    if (points != nullptr) {
        const std::size_t old = points->size();
        points->resize(old + n);

        for (int i = 0; i < n; ++i) {
            (*points)[old + i] = {
                x[i], y[i], z[i], zone_id, i
            };
        }
    }
}

static void skip_connectivity(std::istream& in,
                              const Zone& z) {
    int fields = 0;

    if (z.type == "FETetrahedron") {
        fields = 4;
    } else if (z.type == "FEBrick") {
        fields = 8;
    } else if (z.type == "FETriangle") {
        fields = 3;
    } else if (z.type == "FEQuadrilateral") {
        fields = 4;
    } else {
        throw std::runtime_error(
            "Unsupported zone type: " + z.type);
    }

    long long value;

    for (long long i = 0;
         i < static_cast<long long>(z.elements) * fields;
         ++i) {
        if (!(in >> value)) {
            throw std::runtime_error(
                "EOF in connectivity for " + z.name);
        }
    }
}

static std::unique_ptr<std::istream>
open_gzip(const std::string& path,
          FILE*& pipe,
          std::unique_ptr<PipeBuf>& buffer) {
    const std::string command =
        "gzip -dc \"" + path + "\"";

    pipe = popen(command.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("Could not open gzip stream.");
    }

    buffer = std::make_unique<PipeBuf>(pipe);

    return std::make_unique<std::istream>(buffer.get());
}

static void add_edge(std::vector<Edge>& edges,
                     uint32_t a,
                     uint32_t b) {
    if (a == b) {
        return;
    }

    if (a > b) {
        std::swap(a, b);
    }

    edges.push_back({a, b});
}

static void add_clique(std::vector<Edge>& edges,
                       const std::vector<uint32_t>& nodes) {
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (std::size_t j = i + 1; j < nodes.size(); ++j) {
            add_edge(edges, nodes[i], nodes[j]);
        }
    }
}

static void add_tetra_edges(std::vector<Edge>& edges,
                            const std::vector<uint32_t>& n) {
    add_edge(edges, n[0], n[1]);
    add_edge(edges, n[0], n[2]);
    add_edge(edges, n[0], n[3]);
    add_edge(edges, n[1], n[2]);
    add_edge(edges, n[1], n[3]);
    add_edge(edges, n[2], n[3]);
}

static void add_pyramid_edges(std::vector<Edge>& edges,
                              const std::vector<uint32_t>& n) {
    // Tecplot encoding: a b c d e e e e
    const uint32_t a = n[0];
    const uint32_t b = n[1];
    const uint32_t c = n[2];
    const uint32_t d = n[3];
    const uint32_t e = n[4];

    add_edge(edges, a, b);
    add_edge(edges, b, c);
    add_edge(edges, c, d);
    add_edge(edges, d, a);

    add_edge(edges, a, e);
    add_edge(edges, b, e);
    add_edge(edges, c, e);
    add_edge(edges, d, e);
}

static void add_prism_edges(std::vector<Edge>& edges,
                            const std::vector<uint32_t>& n) {
    // Observed Tecplot encoding:
    // a b c c d e f f
    const uint32_t a = n[0];
    const uint32_t b = n[1];
    const uint32_t c = n[2];
    const uint32_t d = n[4];
    const uint32_t e = n[5];
    const uint32_t f = n[6];

    add_edge(edges, a, b);
    add_edge(edges, b, c);
    add_edge(edges, c, a);

    add_edge(edges, d, e);
    add_edge(edges, e, f);
    add_edge(edges, f, d);

    add_edge(edges, a, d);
    add_edge(edges, b, e);
    add_edge(edges, c, f);
}

static std::vector<uint32_t>
map_local_nodes(const std::vector<int>& local,
                int zone_id,
                const std::vector<std::vector<uint32_t>>& mapping) {
    std::vector<uint32_t> result;
    result.reserve(local.size());

    for (int id : local) {
        if (id <= 0 ||
            id > static_cast<int>(mapping[zone_id].size())) {
            throw std::runtime_error("Invalid local node ID.");
        }

        result.push_back(mapping[zone_id][id - 1]);
    }

    return result;
}

static void write_csr(const std::string& path,
                      uint32_t vertices,
                      const std::vector<Edge>& edges) {
    std::vector<uint64_t> degree(vertices, 0);

    for (const Edge& e : edges) {
        ++degree[e.u];
        ++degree[e.v];
    }

    std::vector<uint64_t> offsets(vertices + 1, 0);

    for (uint32_t v = 0; v < vertices; ++v) {
        offsets[v + 1] = offsets[v] + degree[v];
    }

    std::vector<uint32_t> neighbors(edges.size() * 2);
    std::vector<uint64_t> cursor = offsets;

    for (const Edge& e : edges) {
        neighbors[cursor[e.u]++] = e.v;
        neighbors[cursor[e.v]++] = e.u;
    }

    for (uint32_t v = 0; v < vertices; ++v) {
        std::sort(
            neighbors.begin() + offsets[v],
            neighbors.begin() + offsets[v + 1]
        );
    }

    std::ofstream out(path, std::ios::binary);

    if (!out) {
        throw std::runtime_error(
            "Could not create " + path);
    }

    const uint64_t edge_count = edges.size();

    out.write(
        reinterpret_cast<const char*>(&vertices),
        sizeof(vertices));

    out.write(
        reinterpret_cast<const char*>(&edge_count),
        sizeof(edge_count));

    out.write(
        reinterpret_cast<const char*>(offsets.data()),
        offsets.size() * sizeof(uint64_t));

    out.write(
        reinterpret_cast<const char*>(neighbors.data()),
        neighbors.size() * sizeof(uint32_t));
}

static void first_pass(const std::string& input_path,
                       std::vector<Zone>& zones,
                       std::vector<Point>& points) {
    FILE* pipe = nullptr;
    std::unique_ptr<PipeBuf> buffer;
    auto input = open_gzip(input_path, pipe, buffer);

    std::string line;

    while (std::getline(*input, line)) {
        if (line.find("ZONE T=") == std::string::npos) {
            continue;
        }

        Zone z = read_zone_header(*input, line);

        const int zone_id = static_cast<int>(zones.size());
        zones.push_back(z);

        if (z.nodes > 0) {
            consume_coordinates(
                *input,
                z.nodes,
                &points,
                zone_id);
        }

        if (z.elements > 0) {
            skip_connectivity(*input, z);
        }
    }

    pclose(pipe);
}

static void build_mapping(
    const std::vector<Zone>& zones,
    std::vector<Point>& points,
    std::vector<std::vector<uint32_t>>& mapping) {

    mapping.resize(zones.size());

    for (std::size_t i = 0; i < zones.size(); ++i) {
        mapping[i].resize(zones[i].nodes);
    }

    std::sort(
        points.begin(),
        points.end(),
        [](const Point& a, const Point& b) {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            if (a.z != b.z) return a.z < b.z;
            if (a.zone != b.zone) return a.zone < b.zone;
            return a.local < b.local;
        });

    uint32_t global = 0;

    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i == 0 ||
            points[i].x != points[i - 1].x ||
            points[i].y != points[i - 1].y ||
            points[i].z != points[i - 1].z) {
            ++global;
        }

        mapping[points[i].zone][points[i].local] =
            global - 1;
    }

    std::cout << "Global unique nodes: "
              << global << '\n';
}

static void second_pass(
    const std::string& input_path,
    const std::vector<Zone>& /*zones*/,
    const std::vector<std::vector<uint32_t>>& mapping,
    int criterion,
    std::vector<Edge>& edges) {

    FILE* pipe = nullptr;
    std::unique_ptr<PipeBuf> buffer;
    auto input = open_gzip(input_path, pipe, buffer);

    std::string line;
    int zone_id = -1;

    while (std::getline(*input, line)) {
        if (line.find("ZONE T=") == std::string::npos) {
            continue;
        }

        ++zone_id;

        Zone z = read_zone_header(*input, line);

        // Consume coordinates.
        consume_coordinates(
            *input,
            z.nodes,
            nullptr,
            zone_id);

        const bool use_volume =
            criterion == 1 || criterion == 2;

        const bool use_boundary =
            criterion == 3;

        if ((use_volume && !is_volume(z.name)) ||
            (use_boundary && !is_boundary(z.name))) {
            skip_connectivity(*input, z);
            continue;
        }

        int fields = 0;

        if (z.type == "FETetrahedron") {
            fields = 4;
        } else if (z.type == "FEBrick") {
            fields = 8;
        } else if (z.type == "FETriangle") {
            fields = 3;
        } else if (z.type == "FEQuadrilateral") {
            fields = 4;
        } else {
            throw std::runtime_error(
                "Unsupported zone type: " + z.type);
        }

        std::vector<int> local(fields);

        for (int e = 0; e < z.elements; ++e) {
            for (int j = 0; j < fields; ++j) {
                if (!(*input >> local[j])) {
                    throw std::runtime_error(
                        "EOF in connectivity.");
                }
            }

            std::vector<uint32_t> nodes =
                map_local_nodes(local, zone_id, mapping);

            if (criterion == 1) {
                // Same-element clique.
                std::sort(nodes.begin(), nodes.end());
                nodes.erase(
                    std::unique(nodes.begin(), nodes.end()),
                    nodes.end());

                add_clique(edges, nodes);
            } else if (criterion == 2) {
                if (z.type == "FETetrahedron") {
                    add_tetra_edges(edges, nodes);
                } else if (z.name == "PYRAMIDS") {
                    add_pyramid_edges(edges, nodes);
                } else if (z.name == "PRISMS") {
                    add_prism_edges(edges, nodes);
                }
            } else if (criterion == 3) {
                std::sort(nodes.begin(), nodes.end());
                nodes.erase(
                    std::unique(nodes.begin(), nodes.end()),
                    nodes.end());

                add_clique(edges, nodes);
            }
        }
    }

    pclose(pipe);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr
            << "Usage: " << argv[0]
            << " <input.gz> <criterion> <output.bin>\n"
            << "criterion: 1=same-element, "
            << "2=mesh-edge, 3=boundary-face\n";
        return EXIT_FAILURE;
    }

    const std::string input = argv[1];
    const int criterion = std::stoi(argv[2]);
    const std::string output = argv[3];

    if (criterion < 1 || criterion > 3) {
        std::cerr << "Criterion must be 1, 2, or 3.\n";
        return EXIT_FAILURE;
    }

    try {
        std::vector<Zone> zones;
        std::vector<Point> points;

        points.reserve(5'900'000);

        std::cout << "Pass 1: reading zones and coordinates...\n";

        first_pass(input, zones, points);

        std::cout << "Zones: "
                  << zones.size() << '\n';

        std::size_t total_zone_nodes = 0;

        for (const Zone& z : zones) {
            total_zone_nodes += z.nodes;

            std::cout
                << z.name
                << ": nodes=" << z.nodes
                << ", elements=" << z.elements
                << ", type=" << z.type
                << '\n';
        }

        std::cout
            << "Zone-local node total: "
            << total_zone_nodes << '\n';

        std::vector<std::vector<uint32_t>> mapping;

        std::cout << "\nAssigning global node IDs...\n";

        build_mapping(zones, points, mapping);

        points.clear();
        points.shrink_to_fit();

        std::vector<Edge> edges;

        /*
         * Reserve conservatively. The exact size depends on criterion.
         */
        edges.reserve(40'000'000);

        std::cout
            << "\nPass 2: building criterion "
            << criterion << " edges...\n";

        second_pass(
            input,
            zones,
            mapping,
            criterion,
            edges);

        std::cout
            << "Raw edges: "
            << edges.size() << '\n';

        std::sort(edges.begin(), edges.end());
        edges.erase(
            std::unique(edges.begin(), edges.end()),
            edges.end());

        std::cout
            << "Unique undirected edges: "
            << edges.size() << '\n';

        const uint32_t vertices =
            [&]() {
                uint32_t max_id = 0;

                for (const auto& zone_mapping : mapping) {
                    for (uint32_t id : zone_mapping) {
                        max_id = std::max(max_id, id);
                    }
                }

                return max_id + 1;
            }();

        std::cout
            << "Graph vertices: "
            << vertices << '\n';

        write_csr(output, vertices, edges);

        std::ofstream metadata(
            output + ".txt");

        metadata
            << "criterion=" << criterion << '\n'
            << "vertices=" << vertices << '\n'
            << "edges=" << edges.size() << '\n'
            << "zone_local_nodes=" << total_zone_nodes << '\n';

        std::cout
            << "CSR graph written to: "
            << output << '\n';

    } catch (const std::exception& e) {
        std::cerr
            << "ERROR: "
            << e.what() << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

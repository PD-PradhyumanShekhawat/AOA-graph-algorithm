#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct ZoneInfo {
    std::string name;
    int nodes = 0;
    int elements = 0;
    std::string zone_type;

    std::vector<std::array<int, 8>> first_connectivity;

    int min_node_id = std::numeric_limits<int>::max();
    int max_node_id = std::numeric_limits<int>::min();
};

static bool starts_with(const std::string& line, const std::string& prefix) {
    return line.compare(0, prefix.size(), prefix) == 0;
}

static std::string extract_value(const std::string& line,
                                 const std::string& key) {
    const std::size_t pos = line.find(key);

    if (pos == std::string::npos) {
        return {};
    }

    const std::size_t begin = pos + key.size();
    std::size_t end = line.find(',', begin);

    if (end == std::string::npos) {
        end = line.size();
    }

    std::string value = line.substr(begin, end - begin);

    while (!value.empty() && std::isspace(
               static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }

    while (!value.empty() && std::isspace(
               static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }

    return value;
}

static bool parse_zone_header(const std::string& line, ZoneInfo& zone) {
    if (line.find("ZONE T=") == std::string::npos) {
        return false;
    }

    const std::size_t begin = line.find('"');
    const std::size_t end = line.find('"', begin + 1);

    if (begin == std::string::npos || end == std::string::npos) {
        throw std::runtime_error("Malformed ZONE header.");
    }

    zone = ZoneInfo{};
    zone.name = line.substr(begin + 1, end - begin - 1);

    return true;
}

static void parse_zone_metadata(std::istream& input, ZoneInfo& zone) {
    std::string line;

    bool found_nodes = false;
    bool found_datapacking = false;
    bool found_zone_type = false;

    while (std::getline(input, line)) {
        if (starts_with(line, " Nodes=")) {
            const std::string nodes =
                extract_value(line, "Nodes=");

            const std::string elements =
                extract_value(line, "Elements=");

            const std::string zone_type =
                extract_value(line, "ZONETYPE=");

            if (nodes.empty() || elements.empty() || zone_type.empty()) {
                throw std::runtime_error(
                    "Could not parse zone metadata for " + zone.name);
            }

            zone.nodes = std::stoi(nodes);
            zone.elements = std::stoi(elements);
            zone.zone_type = zone_type;

            found_nodes = true;
            found_zone_type = true;
        }

        if (starts_with(line, " DATAPACKING=")) {
            const std::string packing =
                extract_value(line, "DATAPACKING=");

            if (packing != "BLOCK") {
                throw std::runtime_error(
                    "Expected BLOCK data packing in zone " + zone.name);
            }

            found_datapacking = true;
        }

        /*
         * The coordinates and connectivity follow the metadata.
         * We stop after the DT line because that is the final header line.
         */
        if (starts_with(line, " DT=")) {
            break;
        }
    }

    if (!found_nodes || !found_zone_type || !found_datapacking) {
        throw std::runtime_error(
            "Incomplete metadata for zone " + zone.name);
    }
}

static void read_coordinates(std::istream& input, int count) {
    /*
     * DATAPACKING=BLOCK means:
     *
     * X1 X2 ... XN
     * Y1 Y2 ... YN
     * Z1 Z2 ... ZN
     *
     * We only need to consume the values here.
     *
     * A floating-point token is recognized by attempting to parse
     * the next whitespace-separated token.
     */
    std::string token;

    for (int coordinate_block = 0; coordinate_block < 3;
         ++coordinate_block) {
        for (int i = 0; i < count; ++i) {
            if (!(input >> token)) {
                throw std::runtime_error(
                    "Unexpected end of file while reading coordinates.");
            }
        }
    }
}

static void read_connectivity(std::istream& input, ZoneInfo& zone) {
    /*
     * We know from inspection:
     *
     * TETRAHEDRA -> 4 entries
     * PYRAMIDS   -> 8 entries
     * PRISMS     -> 8 entries
     *
     * The latter two are Tecplot FEBrick records with repeated
     * node IDs representing the physical pyramid/prism.
     */
    int fields_per_element = 0;

    if (zone.zone_type == "FETetrahedron") {
        fields_per_element = 4;
    } else if (zone.name == "PYRAMIDS" || zone.name == "PRISMS") {
        fields_per_element = 8;
    } else {
        throw std::runtime_error(
            "Unexpected volume zone type: " + zone.name +
            " / " + zone.zone_type);
    }

    std::array<int, 8> connectivity{};

    for (int element = 0; element < zone.elements; ++element) {
        for (int j = 0; j < fields_per_element; ++j) {
            if (!(input >> connectivity[j])) {
                throw std::runtime_error(
                    "Unexpected end of file while reading connectivity "
                    "for zone " + zone.name);
            }

            zone.min_node_id =
                std::min(zone.min_node_id, connectivity[j]);

            zone.max_node_id =
                std::max(zone.max_node_id, connectivity[j]);
        }

        if (zone.first_connectivity.size() < 5) {
            zone.first_connectivity.push_back(connectivity);
        }
    }
}

static bool is_boundary_zone(const ZoneInfo& zone) {
    return zone.name.size() >= 2 &&
           zone.name[0] == 'B' &&
           zone.name[1] == '_';
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0]
            << " <HIRENASD_coarse_nc_volume.dat.gz>\n";
        return EXIT_FAILURE;
    }

    const std::string input_path = argv[1];

    /*
     * Use popen() with gzip -dc so the 1 GB uncompressed file
     * never needs to exist on disk.
     */
    const std::string command =
        "gzip -dc \"" + input_path + "\"";

    FILE* pipe = popen(command.c_str(), "r");

    if (pipe == nullptr) {
        std::cerr << "Failed to open gzip stream.\n";
        return EXIT_FAILURE;
    }

    /*
     * FILE* -> std::streambuf adapter.
     *
     * For portability and simplicity in this first inspector,
     * copy the decompressed stream into a temporary stream using
     * std::fread().
     *
     * We do NOT want to buffer the entire file, so instead use
     * a custom stream buffer below.
     */

    class PipeBuf : public std::streambuf {
    private:
        FILE* pipe_;
        char buffer_[64 * 1024];

    public:
        explicit PipeBuf(FILE* pipe)
            : pipe_(pipe) {
            setg(buffer_, buffer_, buffer_);
        }

    protected:
        int_type underflow() override {
            if (gptr() < egptr()) {
                return traits_type::to_int_type(*gptr());
            }

            const std::size_t count =
                std::fread(buffer_, 1, sizeof(buffer_), pipe_);

            if (count == 0) {
                return traits_type::eof();
            }

            setg(buffer_, buffer_, buffer_ + count);

            return traits_type::to_int_type(*gptr());
        }
    };

    PipeBuf pipe_buffer(pipe);
    std::istream input(&pipe_buffer);

    std::vector<ZoneInfo> zones;

    try {
        std::string line;

        while (std::getline(input, line)) {
            if (line.find("ZONE T=") == std::string::npos) {
                continue;
            }

            ZoneInfo zone;

            if (!parse_zone_header(line, zone)) {
                continue;
            }

            parse_zone_metadata(input, zone);

            read_coordinates(input, zone.nodes);

            /*
             * Boundary zones are intentionally inspected only at the
             * metadata level for this first pass. The graph will be
             * constructed from the volume zones.
             */
            if (!is_boundary_zone(zone)) {
                read_connectivity(input, zone);
            }

            zones.push_back(std::move(zone));
        }
    } catch (const std::exception& error) {
        pclose(pipe);

        std::cerr << "ERROR: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    const int exit_status = pclose(pipe);

    if (exit_status != 0) {
        std::cerr << "gzip process failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "\nHIRENASD Tecplot inspection\n";
    std::cout << "===========================\n\n";

    for (const ZoneInfo& zone : zones) {
        std::cout << "Zone: " << zone.name << '\n';
        std::cout << "  Nodes: " << zone.nodes << '\n';
        std::cout << "  Elements: " << zone.elements << '\n';
        std::cout << "  Zone type: " << zone.zone_type << '\n';

        if (!is_boundary_zone(zone)) {
            std::cout << "  Connectivity ID range: "
                      << zone.min_node_id << " - "
                      << zone.max_node_id << '\n';

            std::cout << "  First connectivity records:\n";

            for (const auto& record : zone.first_connectivity) {
                std::cout << "    ";

                if (zone.zone_type == "FETetrahedron") {
                    for (int i = 0; i < 4; ++i) {
                        std::cout << record[i] << ' ';
                    }
                } else {
                    for (int i = 0; i < 8; ++i) {
                        std::cout << record[i] << ' ';
                    }
                }

                std::cout << '\n';
            }
        }

        std::cout << '\n';
    }

    return EXIT_SUCCESS;
}

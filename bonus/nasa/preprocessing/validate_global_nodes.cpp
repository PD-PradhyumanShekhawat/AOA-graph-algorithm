#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

struct Point {
    double x;
    double y;
    double z;

    int zone;
    int local_id;
};

struct ZoneInfo {
    std::string name;
    int nodes = 0;
};

class PipeBuf : public std::streambuf {
private:
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

        const std::size_t count =
            std::fread(buffer_, 1, sizeof(buffer_), pipe_);

        if (count == 0) {
            return traits_type::eof();
        }

        setg(buffer_, buffer_, buffer_ + count);

        return traits_type::to_int_type(*gptr());
    }
};

static bool is_volume_zone(const std::string& name) {
    return name == "TETRAHEDRA" ||
           name == "PYRAMIDS" ||
           name == "PRISMS";
}

static bool is_zone_line(const std::string& line) {
    return line.find("ZONE T=") != std::string::npos;
}

static std::string zone_name(const std::string& line) {
    const std::size_t first = line.find('"');
    const std::size_t second = line.find('"', first + 1);

    if (first == std::string::npos ||
        second == std::string::npos) {
        throw std::runtime_error("Malformed ZONE line.");
    }

    return line.substr(first + 1, second - first - 1);
}

static int extract_int(const std::string& line,
                       const std::string& key) {
    const std::size_t pos = line.find(key);

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

static void read_zone_header(std::istream& input,
                             ZoneInfo& zone) {
    std::string line;

    while (std::getline(input, line)) {
        if (line.find(" Nodes=") == 0) {
            zone.nodes = extract_int(line, "Nodes=");
            break;
        }
    }

    while (std::getline(input, line)) {
        if (line.find(" DT=") == 0) {
            break;
        }
    }
}

static void read_coordinates(std::istream& input,
                             int count,
                             int zone_id,
                             std::vector<Point>& points) {
    /*
     * Tecplot BLOCK format:
     *
     * all X values
     * all Y values
     * all Z values
     *
     * We temporarily store the three coordinates because they are
     * needed to identify shared nodes between zones.
     */
    std::vector<double> x(count);
    std::vector<double> y(count);
    std::vector<double> z(count);

    for (int i = 0; i < count; ++i) {
        if (!(input >> x[i])) {
            throw std::runtime_error(
                "Unexpected EOF while reading X coordinates.");
        }
    }

    for (int i = 0; i < count; ++i) {
        if (!(input >> y[i])) {
            throw std::runtime_error(
                "Unexpected EOF while reading Y coordinates.");
        }
    }

    for (int i = 0; i < count; ++i) {
        if (!(input >> z[i])) {
            throw std::runtime_error(
                "Unexpected EOF while reading Z coordinates.");
        }
    }

    const std::size_t old_size = points.size();
    points.resize(old_size + count);

    for (int i = 0; i < count; ++i) {
        points[old_size + i] = {
            x[i],
            y[i],
            z[i],
            zone_id,
            i + 1
        };
    }
}

static bool same_point(const Point& a, const Point& b) {
    return a.x == b.x &&
           a.y == b.y &&
           a.z == b.z;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0]
            << " <HIRENASD_coarse_nc_volume.dat.gz>\n";
        return EXIT_FAILURE;
    }

    const std::string input_path = argv[1];

    const std::string command =
        "gzip -dc \"" + input_path + "\"";

    FILE* pipe = popen(command.c_str(), "r");

    if (pipe == nullptr) {
        std::cerr << "Failed to start gzip.\n";
        return EXIT_FAILURE;
    }

    PipeBuf pipe_buffer(pipe);
    std::istream input(&pipe_buffer);

    std::vector<Point> points;

    /*
     * Reserve the known volume-zone total:
     * 909018 + 3729 + 4897510 = 5810257
     */
    points.reserve(5'810'257);

    std::vector<ZoneInfo> zones;

    try {
        std::string line;
        int volume_zone_id = 0;

        while (std::getline(input, line)) {
            if (!is_zone_line(line)) {
                continue;
            }

            ZoneInfo zone;
            zone.name = zone_name(line);

            read_zone_header(input, zone);

            if (!is_volume_zone(zone.name)) {
                /*
                 * Boundary zones are irrelevant to the volume graph.
                 * Their coordinates/connectivity are not consumed here,
                 * because this first validator only targets the three
                 * volume zones.
                 */
                continue;
            }

            ++volume_zone_id;

            std::cout
                << "Reading " << zone.name
                << " (" << zone.nodes << " nodes)...\n";

            read_coordinates(
                input,
                zone.nodes,
                volume_zone_id,
                points
            );

            zones.push_back(zone);
        }
    } catch (const std::exception& error) {
        pclose(pipe);

        std::cerr << "ERROR: "
                  << error.what() << '\n';

        return EXIT_FAILURE;
    }

    pclose(pipe);

    std::cout << "\nSorting " << points.size()
              << " volume-zone nodes...\n";

    std::sort(
        points.begin(),
        points.end(),
        [](const Point& a, const Point& b) {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            if (a.z != b.z) return a.z < b.z;
            return a.zone < b.zone;
        }
    );

    std::size_t unique_nodes = 0;
    std::size_t duplicate_nodes = 0;

    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i == 0 || !same_point(points[i], points[i - 1])) {
            ++unique_nodes;
        } else {
            ++duplicate_nodes;
        }
    }

    std::cout << "\nGlobal node validation\n";
    std::cout << "======================\n";

    std::cout << "Zone-local nodes: "
              << points.size() << '\n';

    std::cout << "Unique coordinate nodes: "
              << unique_nodes << '\n';

    std::cout << "Merged duplicate nodes: "
              << duplicate_nodes << '\n';

    std::cout << "\nExpected NASA coarse node count: "
              << "approximately 5.7 million\n";

    return EXIT_SUCCESS;
}

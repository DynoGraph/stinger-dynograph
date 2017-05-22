#include "edge.h"
#include "logger.h"
#include "pvector.h"

using namespace DynoGraph;

int main(int argc, const char* argv[])
{
    Logger& logger = Logger::get_instance();

    // Fixed size buffer of binary edges
    size_t block_size = 1024 * 1024;
    pvector<Edge> edges(block_size);
    pvector<std::string> edge_strings(block_size);

    // Read in edges one block at a time
    while (size_t rc = fread(&edges[0], sizeof(Edge), block_size, stdin))
    {
        if (rc > block_size) {
            logger << "Bad return code from fread()\n";
            die();
        }

        edges.resize(rc);
        edge_strings.resize(rc);

        // Convert edges to strings
        std::transform(edges.begin(), edges.end(), edge_strings.begin(),
            [](const Edge& e) {
                std::ostringstream oss;
                oss << e << "\n";
                return oss.str();
            }
        );

        // Write out strings
        for (const std::string& str : edge_strings)
        {
            fwrite(str.c_str(), sizeof(char), str.size(), stdout);
        }
    }
    return 0;
}


#include "args.h"
#include "helpers.h"
#include "logger.h"
#include <sstream>
#include <getopt.h>
#include <assert.h>

using namespace DynoGraph;
using std::string;
using std::stringstream;

static const option long_options[] = {
    {"num-epochs" , required_argument, 0, 0},
    {"input-path" , required_argument, 0, 0},
    {"batch-size" , required_argument, 0, 0},
    {"alg-names"  , required_argument, 0, 0},
    {"sort-mode"  , required_argument, 0, 0},
    {"window-size", required_argument, 0, 0},
    {"num-trials" , required_argument, 0, 0},
    {"num-alg-trials", required_argument, 0, 0},
    {"sources-path", required_argument, 0, 0},
    {"help"       , no_argument, 0, 0},
    {NULL         , 0, 0, 0}
};

static const std::pair<string, string> option_descriptions[] = {
    {"num-epochs" , "Number of epochs (algorithm updates) in the benchmark"},
    {"input-path" , "File path to the graph edge list to load (.graph.el or .graph.bin)"},
    {"batch-size" , "Number of edges in each batch of insertions"},
    {"alg-names"  , "Algorithms to run in each epoch"},
    {"sort-mode"  , "Controls batch pre-processing: \n"
        "\t\tunsorted (no preprocessing, default),\n"
        "\t\tpresort (sort and deduplicate before insert), or\n "
        "\t\tsnapshot (clear out graph and reconstruct for each batch)"},
    {"window-size", "Percentage of the graph to hold in memory (computed using timestamps) "},
    {"num-trials" , "Number of times to repeat the benchmark"},
    {"num-alg-trials" , "Number of times to repeat algorithms in each epoch"},
    {"sources-path" , "File path to the list of source vertices to use for graph algorithms"},
    {"help"       , "Print help"},
};

void
Args::print_help(string argv0){
    Logger &logger = Logger::get_instance();
    stringstream oss;
    oss << "Usage: " << argv0 << " [OPTIONS]\n";
    for (auto &o : option_descriptions)
    {
        const string &name = o.first;
        const string &desc = o.second;
        oss << "\t--" << name << "\t" << desc << "\n";
    }
    logger << oss.str();
}

Args
Args::parse(int argc, char *argv[])
{
    Logger &logger = Logger::get_instance();
    Args args = {};
    args.sort_mode = Args::SORT_MODE::UNSORTED;
    args.window_size = 1.0;
    args.num_trials = 1;
    args.num_alg_trials = 1;
    args.sources_path = "";

    int option_index;
    while (1)
    {
        int c = getopt_long(argc, argv, "", long_options, &option_index);

        // Done parsing
        if (c == -1) { break; }
        // Parse error
        if (c == '?') {
            logger << "Invalid arguments\n";
            print_help(argv[0]);
            die();
        }
        string option_name = long_options[option_index].name;

        if (option_name == "num-epochs") {
            args.num_epochs = static_cast<int64_t>(std::stoll(optarg));

        } else if (option_name == "alg-names") {
            std::string alg_str = optarg;
            args.alg_names = split(alg_str, ' ');

        } else if (option_name == "input-path") {
            args.input_path = optarg;

        } else if (option_name == "batch-size") {
            args.batch_size = static_cast<int64_t>(std::stoll(optarg));

        } else if (option_name == "sort-mode") {
            std::string sort_mode_str = optarg;
            if      (sort_mode_str == "unsorted") { args.sort_mode = Args::SORT_MODE::UNSORTED; }
            else if (sort_mode_str == "presort")  { args.sort_mode = Args::SORT_MODE::PRESORT;  }
            else if (sort_mode_str == "snapshot") { args.sort_mode = Args::SORT_MODE::SNAPSHOT; }
            else {
                logger << "sort-mode must be one of ['unsorted', 'presort', 'snapshot']\n";
                die();
            }

        } else if (option_name == "window-size") {
            args.window_size = std::stod(optarg);

        } else if (option_name == "num-trials") {
            args.num_trials = static_cast<int64_t>(std::stoll(optarg));

        } else if (option_name == "num-alg-trials") {
            args.num_alg_trials = static_cast<int64_t>(std::stoll(optarg));

        } else if (option_name == "sources-path") {
            args.sources_path = optarg;

        } else if (option_name == "help") {
            print_help(argv[0]);
            die();
        }
    }

    string message = args.validate();
    if (!message.empty())
    {
        logger << "Invalid arguments:\n" + message;
        print_help(argv[0]);
        die();
    }
    return args;
}

string
Args::validate() const
{
    stringstream oss;
    if (num_epochs < 1) {
        oss << "\t--num-epochs must be positive\n";
    }
    if (input_path.empty()) {
        oss << "\t--input-path cannot be empty\n";
    }
    if (batch_size < 1) {
        oss << "\t--batch-size must be positive\n";
    }
    if (window_size < 0 || window_size > 1) {
        oss << "\t--window-size must be in the range [0.0, 1.0]\n";
    }
    if (num_trials < 1) {
        oss << "\t--num-trials must be positive\n";
    }
    if (num_alg_trials < 1) {
        oss << "\t--num-alg-trials must be positive\n";
    }

    return oss.str();
}

std::ostream&
DynoGraph::operator <<(std::ostream& os, Args::SORT_MODE sort_mode)
{
    switch (sort_mode) {
        case Args::SORT_MODE::UNSORTED: os << "unsorted"; break;
        case Args::SORT_MODE::PRESORT: os << "presort"; break;
        case Args::SORT_MODE::SNAPSHOT: os << "snapshot"; break;
        default: os << "UNINITIALIZED"; break;
    }
    return os;
}

std::ostream&
DynoGraph::operator <<(std::ostream& os, const Args& args)
{
    os  << "{"
        << "\"num_epochs\":"  << args.num_epochs << ","
        << "\"input_path\":\""  << args.input_path << "\","
        << "\"batch_size\":"  << args.batch_size << ","
        << "\"window_size\":" << args.window_size << ","
        << "\"num_trials\":"  << args.num_trials << ","
        << "\"num_alg_trials\":"  << args.num_alg_trials << ","
        << "\"sources_path\":" << args.sources_path << ","
        << "\"sort_mode\":\""   << args.sort_mode << "\",";

    os << "\"alg_names\":[";
    for (size_t i = 0; i < args.alg_names.size(); ++i) {
        if (i != 0) { os << ","; }
        os << "\"" << args.alg_names[i] << "\"";
    }
    os << "]}";
    return os;
};
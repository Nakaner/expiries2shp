/*
 * expiries2shp.cpp
 *
 *  Created on: 06.10.2016
 *      Author: michael
 */

#include <getopt.h>
#include <glob.h>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "output_layer.hpp"

void read_expiry_file(std::string& filename, OutputLayer& layer, std::string sequence, bool tile_ids, int min_zoom, int max_zoom) {
    if (sequence == "") {
        sequence = filename.substr(0, filename.find_last_of("."));
    }
    std::ifstream expiryfile;
    expiryfile.open(filename);
    std::string line;
    while (std::getline(expiryfile,line)) {
        std::vector<std::string> elements;
        // splitting the string
        boost::split(elements, line, boost::is_any_of("/"));

        int zoom = std::stoi(elements.at(0));
        if (zoom < min_zoom || zoom > max_zoom) {
            continue;
        }
        int x = std::stoi(elements.at(1));
        int y = std::stoi(elements.at(2));
        layer.write_tile(x, y, zoom, sequence, tile_ids);
    }
    expiryfile.close();
}

/**
 * get vector with the names of all files matching a glob pattern
 */
std::vector<std::string> get_filenames_matching_glob(std::string& pattern) {
    glob_t glob_result;
    int glob_err = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
    switch (glob_err) {
    case GLOB_NOSPACE :
        std::cerr << "ERROR: Glob matching failed for pattern " << pattern << "due to lack of memory.\n";
        exit(1);
    case GLOB_ABORTED :
        std::cerr << "ERROR: Glob matching failed for pattern " << pattern << "due to read error.\n";
        exit(1);
    case GLOB_NOMATCH :
        std::cerr << "ERROR: Glob matching failed for pattern " << pattern << ". No matches found.\n";
        exit(1);
    }
    std::vector<std::string> files;
    for(unsigned int i=0; i<glob_result.gl_pathc; ++i) {
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
}

void print_help() {
    std::cerr << "USAGE:\nexpiries2shp [OPTIONS] [INFILES OUT_FILENAME]\n" \
              << "\nOptions:\n" \
              << "  -f, --format         output format (default 'ESRI Shapefile')\n" \
              << "  -p, --projection     Use other projection than EPSG:3857 for output.\n" \
              << "  -s, --sequence       Sequence number instead of filename without suffix\n" \
              << "  -i, --ids            Add columns with x and y index of the tile\n" \
              << "  -v, --verbose        enable verbose output\n" \
              << "  -z, --min-zoom       Only export tiles with zoom equal or larger than this\n" \
              << "  -Z, --max-zoom       Only export tiles with zoom equal or smaller than this\n" \
              << "                       create this expiry list.\n";
}

std::string get_directory(std::string& path) {
    size_t last_slash = path.find_last_of("/");
    if (last_slash == std::string::npos) {
        return ".";
    } else {
        return path.substr(0, last_slash+1);
    }
}

std::string get_filename(std::string& path) {
    size_t begin = path.find_last_of("/");
    // look for .shp suffix
    size_t end = path.rfind(".shp");
    if (begin == std::string::npos) {
        std::cerr << "ERROR: Output path is a directory but should be a file.\n";
        exit(1);
    } else {
        return path.substr(begin+1, end);
    }
}

int main(int argc, char* argv[]) {
    // parse command line arguments
    static struct option long_options[] = {
        {"format", required_argument, 0, 'f'},
        {"projection", required_argument, 0, 'p'},
        {"sequence", required_argument, 0, 's'},
        {"ids", no_argument, 0, 'i'},
        {"verbose", no_argument, 0, 'v'},
        {"min-zoom", required_argument, 0, 'z'},
        {"max-zoom", required_argument, 0, 'Z'},
        {0, 0, 0, 0}
    };
    int epsg = 3857;
    std::string sequence = "";
    std::string output_format = "ESRI Shapefile";
    bool tile_ids = false;
    bool verbose = false;
    int min_zoom = 0;
    int max_zoom = 25;
    while (true) {
        int c = getopt_long(argc, argv, "f:p:s:ifz:Z:v", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'f':
            output_format = optarg;
            break;
        case 'p':
            epsg = atoi(optarg);
            break;
        case 's':
            sequence = optarg;
            break;
        case 'i':
            tile_ids = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 'z':
            min_zoom = atoi(optarg);
            break;
        case 'Z':
            max_zoom = atoi(optarg);
            break;
        default:
            print_help();
            exit(1);
        }
    }
    int remaining_args = argc - optind;
    std::string input_files_pattern = "";
    std::string output_file = "";
    if (remaining_args != 2) {
        print_help();
        exit(1);
    } else {
        input_files_pattern =  argv[optind];
        output_file = argv[optind+1];
    }

    std::vector<std::string> input_filenames = get_filenames_matching_glob(input_files_pattern);

    // set up output layer
    OutputLayer output_layer (get_directory(output_file), get_filename(output_file), epsg, tile_ids, output_format);

    // read expiry file and write shape file
    for (std::string& filename : input_filenames) {
        if (verbose) {
            std::cout << "Reading file " << filename << "\n";
        }
        read_expiry_file(filename, output_layer, sequence, tile_ids, min_zoom, max_zoom);
    }
}


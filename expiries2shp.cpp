/*
 * expiries2shp.cpp
 *
 *  Created on: 06.10.2016
 *      Author: michael
 */

#include <fstream>
#include <getopt.h>
#include <glob.h>
#include <vector>
#include <memory>
#include <gdal/ogr_geometry.h>
#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include <boost/algorithm/string.hpp>
#include "tile.hpp"

OGRLayer* set_up_layer(std::string&& out_directory, std::string&& layer_name, OGRDataSource* data_source,
        OGRSpatialReference* output_srs, bool tile_ids) {
    OGRLayer *layer = data_source->CreateLayer(layer_name.c_str(), output_srs, wkbPolygon, NULL);
    if (!layer) {
        std::cerr << "Creating layer " << layer_name << " failed.\n";
        exit(1);
    }
    OGRFieldDefn sequence_field("sequence", OFTString);
    sequence_field.SetWidth(9);
    int ogrerr = layer->CreateField(&sequence_field);
    if (ogrerr != OGRERR_NONE) {
        std::cerr << "Creating sequence field failed.\n";
        exit(1);
    }
    OGRFieldDefn zoom_field("zoom", OFTInteger);
    zoom_field.SetWidth(3);
    ogrerr = layer->CreateField(&zoom_field);
    if (ogrerr != OGRERR_NONE) {
        std::cerr << "Creating zoom field failed.\n";
        exit(1);
    }
    if (tile_ids) {
        OGRFieldDefn x_field("x", OFTInteger);
        x_field.SetWidth(8);
        ogrerr = layer->CreateField(&x_field);
        if (ogrerr != OGRERR_NONE) {
            std::cerr << "Creating x field failed.\n";
            exit(1);
        }
        OGRFieldDefn y_field("y", OFTInteger);
        y_field.SetWidth(8);
        ogrerr = layer->CreateField(&y_field);
        if (ogrerr != OGRERR_NONE) {
            std::cerr << "Creating y field failed.\n";
            exit(1);
        }
    }

    std::ofstream file;
    std::string cpgname = out_directory;
    cpgname += layer_name;
    cpgname += ".cpg";
    file.open(cpgname.c_str());
    if (file.fail()) {
        throw std::runtime_error("Can't open shapefile: " + cpgname);
    }
    file << "UTF-8" << std::endl;
    file.close();
    return layer;
}

void read_expiry_file(std::string& filename, OGRLayer* layer, OGRSpatialReference* web_merc, OGRSpatialReference* output_srs,
        std::string sequence, bool tile_ids, int min_zoom, int max_zoom) {
    if (sequence == "") {
        sequence = filename.substr(0, filename.find_last_of("."));
    }
    std::ifstream expiryfile;
    expiryfile.open(filename);
    std::string line;
    layer->StartTransaction();
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
        Tile tile(x, y, zoom);
        std::unique_ptr<OGRPolygon> polygon = tile.get_square();
        OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        if (web_merc->GetEPSGGeogCS() != output_srs->GetEPSGGeogCS()) {
            polygon->assignSpatialReference(web_merc);
            polygon->transformTo(output_srs);
        }
        feature->SetGeometry(polygon.get());
        feature->SetField("sequence", sequence.c_str());
        feature->SetField("zoom", zoom);
        if (tile_ids) {
            feature->SetField("x", x);
            feature->SetField("y", y);
        }
        if (layer->CreateFeature(feature) != OGRERR_NONE) {
            std::cerr << "Failed to create feature.\n";
            exit(1);
        }
        OGRFeature::DestroyFeature(feature);
    }
    layer->CommitTransaction();
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
    size_t last_slash = path.find_last_of("/");
    size_t shp_suffix = path.find_last_of(".shp");
    if (last_slash == std::string::npos) {
        return path.substr(0, shp_suffix);
    } else {
        return path.substr(last_slash+1, shp_suffix).c_str();
    }
}

int main(int argc, char* argv[]) {
    // parse command line arguments
    static struct option long_options[] = {
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
    bool tile_ids = false;
    bool verbose = false;
    int min_zoom = 0;
    int max_zoom = 25;
    while (true) {
        int c = getopt_long(argc, argv, "p:s:ifz:Z:v", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
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

    // create shape file
    OGRRegisterAll();
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("ESRI Shapefile");
    if (!driver) {
        std::cerr << "ESRI Shapefile driver not available.\n";
        exit(1);
    }
    CPLSetConfigOption("SHAPE_ENCODING", "UTF8");

    OGRDataSource* data_source = driver->CreateDataSource(get_directory(output_file).c_str(), NULL);
    if (!data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }
    OGRSpatialReference web_mercator_srs; // only necessary if output SRS is not EPSG:3857
    web_mercator_srs.importFromEPSG(3857);
    OGRSpatialReference output_srs;
    output_srs.importFromEPSG(epsg);
    OGRLayer* layer = set_up_layer(get_directory(output_file), get_filename(output_file), data_source, &output_srs, tile_ids);


    // read expiry file and write shape file
    for (std::string& filename : input_filenames) {
        if (verbose) {
            std::cout << "Reading file " << filename << "\n";
        }
        read_expiry_file(filename, layer, &web_mercator_srs, &output_srs, sequence, tile_ids, min_zoom, max_zoom);
    }

    OGRDataSource::DestroyDataSource(data_source);
    OGRCleanupAll();
}


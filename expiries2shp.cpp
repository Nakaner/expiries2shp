/*
 * expiries2shp.cpp
 *
 *  Created on: 06.10.2016
 *      Author: michael
 */

#include <fstream>
#include <getopt.h>
#include <vector>
#include <memory>
#include <gdal/ogr_geometry.h>
#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include <boost/algorithm/string.hpp>
#include "tile.hpp"

OGRLayer* set_up_layer(std::string& out_directory, std::string& layer_name, OGRDataSource* data_source, OGRSpatialReference* output_srs) {
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
    layer->StartTransaction();

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

void print_help() {
    std::cerr << "USAGE:\nexpiries2shp [OPTIONS] [INFILE OUT_DIRECTORY]\n" \
              << "\nOptions:\n" \
              << "  -p, --projection     Use other projection than EPSG:3857 for output.\n" \
              << "  -s, --sequence       Sequence number of the diff file which was used to\n" \
              << "                       create this expiry list.\n";
}

int main(int argc, char* argv[]) {
    // parse command line arguments
    static struct option long_options[] = {
        {"projection", required_argument, 0, 'p'},
        {"sequence", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    int epsg = 3857;
    std::string sequence = "0";
    while (true) {
        int c = getopt_long(argc, argv, "p:s:", long_options, 0);
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
        default:
            print_help();
            exit(1);
        }
    }
    int remaining_args = argc - optind;
    std::string input_filename = "";
    std::string output_directory = "";
    if (remaining_args != 2) {
        print_help();
        exit(1);
    } else {
        input_filename =  argv[optind];
        output_directory = argv[optind+1];
        if (output_directory[output_directory.size()-1] != '/') { // add missing trailing slash
            output_directory.push_back('/');
        }
    }

    // Do the real work
    std::ifstream expiryfile;
    expiryfile.open(input_filename);

    // create shape file
    OGRRegisterAll();
    OGRSFDriver* driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("ESRI Shapefile");
    if (!driver) {
        std::cerr << "ESRI Shapefile driver not available.\n";
        exit(1);
    }
    CPLSetConfigOption("SHAPE_ENCODING", "UTF8");

    OGRDataSource* data_source = driver->CreateDataSource(output_directory.c_str(), NULL);
    if (!data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }
    OGRSpatialReference web_mercator_srs; // only necessary if output SRS is not EPSG:3857
    web_mercator_srs.importFromEPSG(3857);
    OGRSpatialReference output_srs;
    output_srs.importFromEPSG(epsg);
    OGRLayer* layer = set_up_layer(output_directory, sequence, data_source, &output_srs);


    // read expiry file and write shape file
    std::string line;
    while (std::getline(expiryfile,line)) {
        std::vector<std::string> elements;
        // splitting the string
        boost::split(elements, line, boost::is_any_of("/"));

        int zoom = std::stoi(elements.at(0));
        Tile tile(std::stoi(elements.at(1)), std::stoi(elements.at(2)), zoom);
        std::unique_ptr<OGRPolygon> polygon = tile.get_square();
        OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        if (epsg != 3857) {
            polygon->assignSpatialReference(&web_mercator_srs);
            polygon->transformTo(&output_srs);
        }
        feature->SetGeometry(polygon.get());
        feature->SetField("sequence", sequence.c_str());
        feature->SetField("zoom", zoom);
        if (layer->CreateFeature(feature) != OGRERR_NONE) {
            std::cerr << "Failed to create feature.\n";
            exit(1);
        }
        OGRFeature::DestroyFeature(feature);
    }
    layer->CommitTransaction();
    OGRDataSource::DestroyDataSource(data_source);
    OGRCleanupAll();
    expiryfile.close();
}


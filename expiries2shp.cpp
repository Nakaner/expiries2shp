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

const double EARTH_CIRCUMFERENCE = 40075016.68;

int main(int argc, char* argv[]) {
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
            exit(1);
        }
    }

    int remaining_args = argc - optind;
    std::string input_filename = "";
    std::string output_filename = "";
    if (remaining_args != 2) {
            std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE OUT_DIRECTORY]" << std::endl;
            exit(1);
    } else {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
        if (output_filename[output_filename.size()-1] != '/') { // add missing trailing slash
            output_filename.push_back('/');
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

    OGRDataSource* data_source = driver->CreateDataSource(output_filename.c_str(), NULL);
    if (!data_source) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }
    OGRSpatialReference output_srs;
    output_srs.importFromEPSG(epsg);
    OGRLayer *layer = data_source->CreateLayer(sequence.c_str(), &output_srs, wkbPolygon, NULL);
    if (!layer) {
        std::cerr << "Creating layer " << output_filename << " failed.\n";
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
    OGRFieldDefn x_field("x", OFTInteger);
    x_field.SetWidth(7);
    ogrerr = layer->CreateField(&x_field);
    if (ogrerr != OGRERR_NONE) {
        std::cerr << "Creating x field failed.\n";
        exit(1);
    }
    OGRFieldDefn y_field("y", OFTInteger);
    y_field.SetWidth(3);
    ogrerr = layer->CreateField(&y_field);
    if (ogrerr != OGRERR_NONE) {
        std::cerr << "Creating y field failed.\n";
        exit(1);
    }
    layer->StartTransaction();

    std::ofstream file;
    std::string cpgname = output_filename;
    cpgname += sequence;
    cpgname += ".cpg";
    file.open(cpgname.c_str());
    if (file.fail()) {
        throw std::runtime_error("Can't open shapefile: " + cpgname);
    }
    file << "UTF-8" << std::endl;
    file.close();

    std::string line;
    while (std::getline(expiryfile,line)) {
        std::vector<std::string> elements;
        // splitting the string
        boost::split(elements, line, boost::is_any_of("/"));

        int zoom = std::stoi(elements.at(0));
        int x = std::stoi(elements.at(1));
        int y = std::stoi(elements.at(2));
        Tile tile(x, y, zoom);
        std::unique_ptr<OGRPolygon> polygon = tile.get_square();
        OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        feature->SetGeometry(polygon.get());
        feature->SetField("sequence", sequence.c_str());
        feature->SetField("zoom", zoom);
        feature->SetField("x", x);
        feature->SetField("y", y);
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


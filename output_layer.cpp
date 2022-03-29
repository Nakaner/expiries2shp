/*
 * output_layer.cpp
 *
 *  Created on: 11.10.2016
 *      Author: michael
 *
 *  This file contains code from osmium library v1 (include/osmium/export/shapefile.hpp).
 */

#include <memory>
#include "output_layer.hpp"

OutputLayer::OutputLayer(std::string&& out_directory, std::string&& layer_name, int output_epsg, bool tile_ids,
            std::string& output_format) :
            m_current_index(0),
            m_directory(out_directory),
            m_layer_name(layer_name),
            m_tile_ids(tile_ids),
            m_output_format(output_format) {
    m_output_srs.importFromEPSG(output_epsg);
    m_web_mercator_srs.importFromEPSG(3857);
    setup_data_source();
    open_layer();
}

OutputLayer::~OutputLayer() {
	GDALClose(m_dataset);
    OGRCleanupAll();
}

void OutputLayer::add_field(const std::string& field_name, OGRFieldType type, int width) {
    OGRFieldDefn sequence_field(field_name.c_str(), type);
    m_dbf_entry_size += width;
    m_current_dbf_size += 32;
    sequence_field.SetWidth(width);
    int ogrerr = m_layer->CreateField(&sequence_field);
    if (ogrerr != OGRERR_NONE) {
        std::cerr << "Creating " << field_name << " field failed.\n";
        exit(1);
    }
}

void OutputLayer::close_layer() {
	if (m_layer->CommitTransaction() != OGRERR_NONE) {
	    std::cerr << "Error commit transation\n";
	    exit(1);
	}
    m_layer->SyncToDisk();
}

void OutputLayer::open_layer() {
    m_current_index++;
    m_current_dbf_size = 33;
    m_dbf_entry_size = 1;

    std::string current_layer_name = m_layer_name;
    current_layer_name += "_";
    current_layer_name += std::to_string(m_current_index);
    m_layer = m_dataset->CreateLayer(current_layer_name.c_str(), &m_output_srs, wkbPolygon, NULL);
    if (!m_layer) {
        std::cerr << "Creating layer " << m_layer_name << " failed.\n";
        exit(1);
    }

    add_field("sequence", OFTString, 12);
    add_field("zoom", OFTInteger, 3);
    if (m_tile_ids) {
        add_field("x", OFTInteger, 8);
        add_field("y", OFTInteger, 8);
    }

    if (m_output_format == "ESRI Shapefile") {
        write_cpg_file();
    }

    m_current_shp_size = 100;

    if (m_layer->StartTransaction() != OGRERR_NONE) {
        std::cerr << "Error starting transaction\n";
        exit(1);
    }
}

void OutputLayer::write_cpg_file() {
    std::ofstream file;
    std::string cpgname = m_directory;
    cpgname += m_layer_name;
    cpgname += "_";
    cpgname += std::to_string(m_current_index);
    cpgname += ".cpg";
    file.open(cpgname.c_str());
    if (file.fail()) {
        throw std::runtime_error("Can't open shapefile: " + cpgname);
    }
    file << "UTF-8" << std::endl;
    file.close();
}

void OutputLayer::setup_data_source() {
    OGRRegisterAll();
    GDALDriver* m_driver = GetGDALDriverManager()->GetDriverByName(m_output_format.c_str());
    if (!m_driver) {
        std::cerr << "Driver for " << m_output_format << " is not available.\n";
        exit(1);
    }
    if (m_output_format == "ESRI Shapfile") {
        CPLSetConfigOption("SHAPE_ENCODING", "UTF8");
    }

    m_dataset = m_driver->Create(m_directory.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (!m_dataset) {
        std::cerr << "Creation of output file failed.\n";
        exit(1);
    }
}

void OutputLayer::set_directory(std::string& path) {
    size_t last_slash = path.find_last_of("/");
    if (last_slash == std::string::npos) {
        m_directory = ".";
    } else {
        m_directory = path.substr(0, last_slash+1);
    }
}

void OutputLayer::set_filename(std::string& path) {
    size_t last_slash = path.find_last_of("/");
    size_t shp_suffix = path.find_last_of(".shp");
    if (last_slash == std::string::npos) {
        m_layer_name = path.substr(0, shp_suffix);
    } else {
        m_layer_name = path.substr(last_slash+1, shp_suffix).c_str();
    }
}

void OutputLayer::ensure_layer_writeable() {
    if (m_layer)
    {
        if ((m_current_dbf_size < m_max_shape_size) && (m_current_shp_size < m_max_shape_size)) return;
        close_layer();
    }
    open_layer();
}

void OutputLayer::write_tile(int x, int y, int zoom, std::string& sequence, bool tile_ids) {
    ensure_layer_writeable();
    Tile tile(x, y, zoom);
    std::unique_ptr<OGRPolygon> polygon = tile.get_square();
    OGRFeature* feature = OGRFeature::CreateFeature(m_layer->GetLayerDefn());
    if (m_web_mercator_srs.GetEPSGGeogCS() != m_output_srs.GetEPSGGeogCS()) {
        polygon->assignSpatialReference(&m_web_mercator_srs);
        polygon->transformTo(&m_output_srs);
    }
    feature->SetGeometry(polygon.get());
    feature->SetField("sequence", sequence.c_str());
    feature->SetField("zoom", zoom);
    if (tile_ids) {
        feature->SetField("x", x);
        feature->SetField("y", y);
    }
    if (m_layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cerr << "Failed to create feature.\n";
        exit(1);
    }
    OGRFeature::DestroyFeature(feature);
    m_current_shp_size += 120;
    m_current_dbf_size += m_dbf_entry_size;
}

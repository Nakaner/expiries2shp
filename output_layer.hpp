/*
 * output_layer.hpp
 *
 *  Created on: 11.10.2016
 *      Author: michael
 *
 *  This file contains code from osmium library v1 (include/osmium/export/shapefile.hpp).
 */

#ifndef OUTPUT_LAYER_HPP_
#define OUTPUT_LAYER_HPP_

#include <iostream>
#include <fstream>
#include <gdal/ogr_geometry.h>
#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include "tile.hpp"

class OutputLayer {
private:
    //current shapefile suffix (for overflow cases)
    int m_current_index;

    std::string m_directory;
    std::string m_layer_name;
    OGRSpatialReference m_output_srs;
    OGRSpatialReference m_web_mercator_srs; // only necessary if output SRS is not EPSG:3857
    bool m_tile_ids;
    std::string& m_output_format;

    OGRSFDriver* m_driver;
    OGRDataSource* m_data_source;

    // counters and constants for calculation of current SHP and DBF file size
    size_t m_current_dbf_size = 0;
    size_t m_current_shp_size = 0;
    size_t m_dbf_entry_size = 0;

    // pointer to current OGRLayer
    OGRLayer* m_layer;

    static const int m_max_shape_size = 2040109465;

    void setup_data_source();

    void set_directory(std::string& path);

    void set_filename(std::string& path);

    /**
     * add a field to a layer
     */
    void add_field(const std::string& field_name, OGRFieldType type, int width);

    /**
     * close the layer
     */
    void close_layer();

    /**
     * open a new layer
     */
    void open_layer();

    /**
     * Write CPG file because GDAL's Shapefile driver does not do it.
     */
    void write_cpg_file();

    /**
     * Check if layer is smaller than 2 GB. If not, close it and reopen a new one.
     */
    void ensure_layer_writeable();

public:
    OutputLayer(std::string&& out_directory, std::string&& layer_name, int output_epsg, bool tile_ids,
            std::string& output_format);

    ~OutputLayer();

    /**
     * write a tile to the output layer
     */
    void write_tile(int x, int y, int zoom, std::string& sequence, bool tile_ids);


};



#endif /* OUTPUT_LAYER_HPP_ */

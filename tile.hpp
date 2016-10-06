/*
 * tile.hpp
 *
 *  Created on: 06.10.2016
 *      Author: michael
 */

#ifndef TILE_HPP_
#define TILE_HPP_

#include <memory>
#include <ogr_geometry.h>

class Tile {
private:
    double m_x; // x coordinate of south west corner in Mercator
    double m_y; // y coordinate of south west corner in Mercator
    double m_width; // width and height of the tile in Mercator units

    const double EARTH_CIRCUMFERENCE = 40075016.68;

    /**
     * convert x index of a tile into the Mercator x coordinate of its south-western corner
     *
     * @param tile_x x coordinate
     * @param map_width number of tiles in any direction at this zoom level
     */
    double tile_x_to_merc(double tile_x, int map_width);

    /**
     * convert x index of a tile into the Mercator y coordinate of its south-western corner
     *
     * @param tile_y y coordinate
     * @param map_width number of tiles in any direction at this zoom level
     */
    double tile_y_to_merc(double tile_y, int map_width);

public:
    /**
     * create an instance using x, y and z index of a tile
     */
    Tile(int tile_x, int tile_y, int zoom);

    /**
     * return the square represented by this tile as OGRPolygon
     */
    std::unique_ptr<OGRPolygon> get_square();
};



#endif /* TILE_HPP_ */

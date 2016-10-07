/*
 * tile.cpp
 *
 *  Created on: 06.10.2016
 *      Author: michael
 */

#include "tile.hpp"
#include <iostream>

Tile::Tile(int tile_x, int tile_y, int zoom) {
    int map_width = 1 << zoom;
    m_x = tile_x_to_merc(tile_x, map_width);
    m_y = tile_y_to_merc(tile_y, map_width);
    m_width = EARTH_CIRCUMFERENCE / map_width;
}

double Tile::tile_x_to_merc(double tile_x, int map_width) {
    return EARTH_CIRCUMFERENCE * ((tile_x/map_width) - 0.5);
}


double Tile::tile_y_to_merc(double tile_y, int map_width) {
    return EARTH_CIRCUMFERENCE * (0.5 - tile_y/map_width);
}

std::unique_ptr<OGRPolygon> Tile::get_square() {
    std::unique_ptr<OGRLinearRing> ring  (new OGRLinearRing());
    ring->addPoint(m_x, m_y);
    ring->addPoint(m_x + m_width, m_y);
    ring->addPoint(m_x + m_width, m_y - m_width);
    ring->addPoint(m_x, m_y - m_width);
    std::unique_ptr<OGRPolygon> polygon (new OGRPolygon());
    polygon->addRingDirectly(ring.release());
    return polygon;
}

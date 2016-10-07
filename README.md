expiries2shp
============

This tool converst a tile expiry list as created by osm2pgsql. It creates a shape file
which contains all expired tiles as polygons.

Building
--------

expiries2shp needs GDAL and Boost.

Ubuntu packages: libboost libgdal

You might adapt the include pathes used in the Makefile.

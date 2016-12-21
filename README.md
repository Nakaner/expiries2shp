expiries2shp
============

This tool converts a tile expiry list which was created by [osm2pgsql](https://github.com/openstreetmap/osm2pgsql)
or [another tool](http://wiki.openstreetmap.org/wiki/Tile_expire_methods) which creates such lists into a shape file.
The shape file contains all expired tiles as polygons. Further processing can be done with any
software you want to use.

This tool ensures that the created shape files are smaller than 2 GB because some software is unable
to read larger shape files. If the shape file to be created is larger than 2 GB, an appendix file is
created whose file name has an suffix like `_2`, `_3` etc.

Building
--------

expiries2shp needs GDAL and Boost.

Ubuntu:

    sudo apt-get install libboost libgdal-dev

You might adapt the include pathes used in the Makefile.


Usage
-----

Build it and run it without any arguments.


Further Processing
------------------

This sections gives examples what you can do with this tool.

If you want to analyse the editing activity in OpenStreetMap, you can create tile expiry lists and
build a large shape file which contains them. Note that the shape file which contains the expired
tiles of one month might have a size of about 1 GB.

To create a heat map of all expired tiles at zoom level 12, import the shape file into a PostGIS database using shp2pgsql and run
following SQL query afterwards (`expiry` is the table with the data of the shape file):

    SELECT count(geom), x, y, zoom, geom INTO expiry_heatmap FROM expiry WHERE zoom = 12 GROUP BY x, y, zoom, geom;

License
-------

This program is available under the terms of GNU GPL v3 or later.
See third-party.md and LICENSE.md for details.

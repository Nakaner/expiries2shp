expiries2shp
============

This tool converts a tile expiry list into a shape file.
Those lists can be created using the tile expiry export of [osm2pgsql](https://github.com/openstreetmap/osm2pgsql) or
[any other tool](http://wiki.openstreetmap.org/wiki/Tile_expire_methods) for that purpose.
You can use expiries2shp to convert a [list of tiles ordered by node count](https://github.com/osmcode/osmium-contrib/tree/master/dense_tiles) into a shape file.

The shape file will contian all tiles as polygons. Further processing can be done with any
software you want to use.

This tool ensures that the created shape files are smaller than 2 GB because some software is unable
to read larger shape files. If the shape file to be created is larger than 2 GB, an appendix file is
created whose file name has an suffix like `_2`, `_3` etc.

Building
--------

expiries2shp needs GDAL and Boost.

Ubuntu/Debian:

    sudo apt-get install libboost-dev libgdal-dev build-essential

You might adapt the include pathes used in the Makefile.

Afterwards you can build it using

    make


Usage
-----

```
expiries2shp [OPTIONS] INPUT_FILE OUTPUT_FILE
  INPUT_FILE is the file to read from or a glob pattern.
  OUTPUT_FILE is the file to write to.
Options:
  -f, --format                  output format (default 'ESRI Shapefile')
  -p, --projection              Use other projection than EPSG:3857 for output.
  -s, --sequence                Sequence number instead of filename without suffix
  -i, --ids                     Add columns with x and y index of the tile
  -m NUM, --metatile-size NUM   Use if input list contains top left tiles of metatiles only.
                                Specify meta tile size.
  -v, --verbose                 enable verbose output
  -z, --min-zoom                Only export tiles with zoom equal or larger than this
  -Z, --max-zoom                Only export tiles with zoom equal or smaller than this
                                create this expiry list.
```

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

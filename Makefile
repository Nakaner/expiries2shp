# Makefile for expiries2shp
# # ----------------------
#
INCLUDES=-I/usr/include/gdal
CXXFLAGS=-std=c++11  -Wall -Wextra  -O3 -g
# #-fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls
LIBS=-lgdal
#
expiries2shp: expiries2shp.o tile.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) -o $@ $<

clean: 
	rm -f expiries2shp *.o

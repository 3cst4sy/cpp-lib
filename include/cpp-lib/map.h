//
// Copyright 2015 KISS Technologies GmbH, Switzerland
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Component: MAP
//
// Mapping/cartography:
// * Projections
// * Tile mapping (Web Mercator)
//


#ifndef CPP_LIB_MAP_H
#define CPP_LIB_MAP_H

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

#include "png.hpp"

#include "cpp-lib/gnss.h"
#include "cpp-lib/registry.h"
#include "cpp-lib/util.h"

#include "cpp-lib/sys/file.h"
#include "cpp-lib/sys/syslogger.h"

#include "boost/multi_array.hpp"


namespace cpl {

namespace map {

// Global Wep Mercator coordinates as per
// https://en.wikipedia.org/wiki/Web_Mercator#Formulas
// 0 <= x, y < tilesize * 2^zoom
struct global_coordinates {
  double x = 0;
  double y = 0;
};

// Coordinates specifying a tile and a pixel within a tile 
// Tile coordinates in [0, 2^zoom)
struct tile_coordinates {
  long x = 0;
  long y = 0;

  tile_coordinates(long const x, long const y) 
  : x(x), y(y) {}

  tile_coordinates()
  {}
};


struct tc_less {
// Strict lexicographical ordering---TODO: Test
bool operator()(tile_coordinates const& tc1, tile_coordinates const& tc2) 
  const {
  if (tc1.x < tc2.x) { return true; }
  if (tc1.x == tc2.x) {
    return tc1.y < tc2.y;
  } else {
    return false;
  }
}
};

// Pixel coordinates in [0, tilesize)
struct pixel_coordinates {
  int x = 0;
  int y = 0;
};

struct full_coordinates {
   tile_coordinates  tile;
  pixel_coordinates pixel;
};

inline int default_tilesize() {
  return 256;
}


// TODO: Use gnss bounding box struct here
struct tileset_parameters {

  tileset_parameters() {}

  tileset_parameters(cpl::gnss::lat_lon const& nw,
                 cpl::gnss::lat_lon const& se,
                 int minzoom, int maxzoom,
                 int tilesize = default_tilesize())
  : minzoom(minzoom),
    maxzoom(maxzoom),
    north_west(nw),
    south_east(se),
    tilesize(tilesize)
  {
    validate();
  }

  void validate() const {
    cpl::gnss::validate_lat_lon(north_west);
    cpl::gnss::validate_lat_lon(south_east);
    cpl::util::verify(minzoom <= maxzoom, 
                      "min zoom must be <= max zoom");
    cpl::util::verify(0 <= minzoom, "min zoom must be >= 0");
    cpl::util::verify(maxzoom <= 32, "max zoom must be <= 32");

    cpl::util::verify(   north_west.lat > south_east.lat 
                      && north_west.lon < south_east.lon,
        "NW corner of tileset must be north/west of SE");
  }

  // Min, max zoom level
  int minzoom = 1;
  int maxzoom = 10;

  // Corners
  cpl::gnss::lat_lon north_west = cpl::gnss::lat_lon{47.8, 4.8};
  cpl::gnss::lat_lon south_east = cpl::gnss::lat_lon{43.7, 12};

  // Tile horizontal and vertical size [pixels]
  int tilesize = default_tilesize();

  // Name for logging
  std::string tileset_name = "unnamed tileset";

  // Default directory to store tiles under
  std::string tile_directory;

  // Empty tile name
  std::string empty_tile_name = "empty.png";
};

tileset_parameters tileset_parameters_from_registry(
    cpl::util::registry const& reg,
    tileset_parameters const& defaults = tileset_parameters{});

// Web Mercator coordinate transformations
// TODO: Should be named tile_coordinate_mapper
struct tile_mapper {
  // Construct with given tile size, power of 2 between 256 and 2048
  tile_mapper(int tilesize = default_tilesize());

  tile_mapper(tileset_parameters const& params)
  : tile_mapper(params.tilesize) {}

  // Returns: Global coordinates at given zoom level
  // Zoom: 10 for thermal heat maps
  global_coordinates get_global_coordinates(
      int zoom, cpl::gnss::lat_lon const& ll) const;

  // Gets tile coordinates for given global coordinates.
  tile_coordinates get_tile_coordinates(
      global_coordinates const& gc) const;

  // Gets tile coordinates for given global coordinates.
  tile_coordinates get_tile_coordinates(
      int const zoom, cpl::gnss::lat_lon const& ll) const {
    return get_tile_coordinates(get_global_coordinates(zoom, ll));
  }

  // Gets pixel coordinates for given global coordinates.  If already
  // computed, supply the tile coordinates to improve performance.
  pixel_coordinates get_pixel_coordinates(
      global_coordinates const& gc, tile_coordinates const& tc) const;

  full_coordinates get_full_coordinates(
      int const zoom,
      cpl::gnss::lat_lon const& ll) const {
    auto const gc = get_global_coordinates(zoom, ll);
    auto const tc = get_tile_coordinates(gc);
    return full_coordinates{tc, get_pixel_coordinates(gc, tc)}; 
  }

  // Returns: tile size
  int tilesize() const { return tilesize_; }

private:
  int tilesize_;
};

typedef tile_mapper tile_coordinate_mapper;

//
// Supports:
// * Automatic on-demand allocation of tiles
// * Access to pixels based on lat/lon and zoom level
// * Iteration over all tiles in square, providing
//   information of whether tile is allocated or not
// * Multiple 'views' of tilesets with different translations
//   of values to RGBA values
//
// TODO:
// * Iterator type(s) to avoid nested for loops
//
template <typename T> struct tileset {
  typedef T value_type;
  typedef boost::multi_array<value_type, 2> tile_type;
  typedef std::map<tile_coordinates, tile_type, tc_less> level_map_type;

  typedef std::function<png::rgba_pixel(value_type)> rgba_function;

  tileset(tileset_parameters const& params)
  : params{params},
    levels{static_cast<unsigned>(maxzoom() - minzoom() + 1)},
    default_tile_(boost::extents[tilesize()][tilesize()])
  {}

  // Flushes tiles to directory, logging to sl and converting pixels 
  // as per rgba_function
  void flush_tiles(std::ostream& sl, rgba_function const&,
      std::string const& directory, 
      std::function<double()> const& timer = cpl::util::utc) const;

  // Simulate default arguments by a second overload
  void flush_tiles(std::ostream& sl, rgba_function const& f,
      std::function<double()> const& timer = cpl::util::utc) const {
      flush_tiles(sl, f, default_tile_directory(), timer);
  }

  // Writes the given tile to filename, converting pixels as per rgba_function
  /*void write_tile(tile_type const& tile, std::string const& filename, 
       rgba_function const&) const;*/

  // Get tile at tile coords/zoom level, if allocated
  tile_type const* tile_at(int const zoom, tile_coordinates const& tc) const {
    auto const& level = level_at(zoom);
    auto const it = level.find(tc);
    if (level.end() == it) {
      return NULL;
    } else {
      return &it->second;
    }
  }

  tile_type      * tile_at(int const zoom, tile_coordinates const& tc)       {
    auto const& this_const = *this;
    return const_cast<tile_type*>(this_const.tile_at(zoom, tc));
  }

  tile_type const*
  tile_at(int const zoom, cpl::gnss::lat_lon const& ll) const {
    return tile_at(zoom, mapper().get_tile_coordinates(zoom, ll));
  }

  tile_type const& default_tile() const { return default_tile_; }

  void erase_tile(int const zoom, tile_coordinates const& tc) {
    level_at(zoom).erase(tc);
  }

  // Returns given tile, creating it if not present.
  tile_type& tile_at_create(int const zoom, tile_coordinates const& tc) {
    auto& level = level_at(zoom);
    auto& tile = level[tc];
    if (0 == tile.size()) {
      tile.resize(boost::extents[tilesize()][tilesize()]);
      assert(0 != tile.size());
    }
    return tile;
  }

  // Returns value at given tile coordinates, creating tile if necessary
  value_type& value_at_create(int const zoom, full_coordinates const& fc) {
    return tile_at_create(zoom, fc.tile)[fc.pixel.x][fc.pixel.y];
  }

  // Returns value at given lat/lon, creating tile if necessary
  value_type& value_at_create(int const zoom, cpl::gnss::lat_lon const& ll) {
    return value_at_create(zoom, mapper().get_full_coordinates(zoom, ll));
  }

  // Returns the mapper, tile size etc.
  tile_mapper const& mapper() const { return tm; }
  int tilesize() const { return mapper().tilesize(); }


  // Get corners at given zoom level
  tile_coordinates north_west_tile(int const zoom) const {
    return tm.get_tile_coordinates(zoom, north_west());
  }
  tile_coordinates south_east_tile(int const zoom) const {
    return tm.get_tile_coordinates(zoom, south_east());
  }

  cpl::gnss::lat_lon north_west() const { return params.north_west; }
  cpl::gnss::lat_lon south_east() const { return params.south_east; }

  // Is the given point in the rectangle?
  bool inside(cpl::gnss::lat_lon const& ll) const {
    return    north_west().lat >= ll.lat && ll.lat >= south_east().lat
           && north_west().lon <= ll.lon && ll.lon <= south_east().lon;
  }

  int minzoom() const { return params.minzoom; }
  int maxzoom() const { return params.maxzoom; }

  std::string const& default_tile_directory() const {
    return parameters().tile_directory;
  }

  double n_allocated_tiles() const {
    double n = 0;
    for (auto const& lvl : levels) {
      n += lvl.size();
    }
    return n;
  }

  double n_max_tiles() const {
    double n = 0;
    for (int z = minzoom(); z <= maxzoom(); ++z) {
      auto const nw = north_west_tile(z);
      auto const se = south_east_tile(z);
      n += (se.x - nw.x + 1) * (se.y - nw.y + 1);
    }
    return n;
  }

  // Storage estimates
  // TODO: Only valid for PODs
  double value_storage() const { return sizeof(value_type); }

  // Storage per tile
  double tile_storage() const {
    return value_storage() * std::pow(tilesize(), 2);
  }

  // Maximum storage
  double max_storage() const {
    return n_max_tiles() * tile_storage();
  }

  // Storage currently allocated (dynamic)
  double allocated_storage() const {
    return n_allocated_tiles() * tile_storage();
  }

  // Returns parameters
  tileset_parameters const& parameters() const {
    return params;
  }

private:

  int level_index(int const zoom) const {
    return zoom - params.minzoom;
  }
  level_map_type&       level_at(int const zoom) 
  { return levels[level_index(zoom)]; }

  level_map_type const& level_at(int const zoom) const
  { return levels[level_index(zoom)]; }

  tileset_parameters params;

  tile_mapper tm;

  std::vector<level_map_type> levels;
  tile_type default_tile_;
};

template<typename T>
std::ostream& write_static_info(
    std::ostream& os, tileset<T> const& ts) {
  using namespace cpl::util::log;
  os << prio::NOTICE 
     << "Static info for tileset: " << ts.parameters().tileset_name
     << std::endl;
  os << prio::NOTICE << "Tile directory: " << ts.parameters().tile_directory
     << std::endl;
  os << prio::NOTICE << "Northwest corner: " << ts.north_west() << std::endl;
  os << prio::NOTICE << "Southeast corner: " << ts.south_east() << std::endl;
  os << prio::NOTICE << "Min zoom level: " << ts.minzoom() << std::endl;
  os << prio::NOTICE << "Max zoom level: " << ts.maxzoom() << std::endl;
  os << prio::NOTICE 
     << "Value size [Bytes]: " << ts.value_storage() << std::endl;
  os << prio::NOTICE << "Storage estimate per tile [MB]: "
     << ts.tile_storage() / 1e6 << std::endl;
  os << prio::NOTICE << "Maximum possible storage estimate [MB]: "
     << ts.max_storage() / 1e6 << std::endl;
  os << prio::NOTICE << "Maximum possible number of tiles: "
     << ts.n_max_tiles() << std::endl;

  os << prio::NOTICE << "Tile coordinate limits:" << std::endl;
  for (int z = ts.minzoom(); z <= ts.maxzoom(); ++z) {
    auto const nw = ts.north_west_tile(z);
    auto const se = ts.south_east_tile(z);
    os << prio::NOTICE << "Zoom " << z << ": "
       << nw.x << " <= x && x <= " << se.x << " && "
       << nw.y << " <= y && y <= " << se.y << std::endl;
  }
  return os;
}

template<typename T>
std::ostream& write_dynamic_info(
    std::ostream& os, tileset<T> const& ts) {
  using namespace cpl::util::log;
  os << prio::NOTICE
     << "Dynamic info for tileset: " << ts.parameters().tileset_name 
     << std::endl;
  os << prio::NOTICE
     << "Number of allocated tiles: " << ts.n_allocated_tiles() << " ("
     << 100 * ts.n_allocated_tiles() / ts.n_max_tiles() << "%)"
     << std::endl;
  os << prio::NOTICE
     << "Allocated storage estimate [MB]: " 
     << ts.allocated_storage() / 1e6 << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream&, global_coordinates const&);
std::ostream& operator<<(std::ostream&,   tile_coordinates const&);
std::ostream& operator<<(std::ostream&,  pixel_coordinates const&);
std::ostream& operator<<(std::ostream&,   full_coordinates const&);


} // namespace map

} // namespace cpl

// Template definitions
template<typename T>
void cpl::map::tileset<T>::flush_tiles(
    std::ostream& sl, 
    rgba_function const& pix,
    std::string const& directory,
    std::function<double()> const& timer) const {
  using namespace cpl::util::log;
  namespace cpf = cpl::util::file;

  try {

  double const start = timer();

  cpf::dir_sentry const s;
  cpf::chdir(directory);
  sl << prio::NOTICE << "Flushing tiles to " << directory
     << std::endl;

  cpl::map::write_dynamic_info(sl, *this); 
  cpf::unlink(params.empty_tile_name, true);
  //write_tile(default_tile(), params.empty_tile_name, pix);

  for (int z = minzoom(); z <= maxzoom(); ++z) {
    auto const nw = north_west_tile(z);
    auto const se = south_east_tile(z);

    for (int y = nw.y; y <= se.y; ++y) {
    for (int x = nw.x; x <= se.x; ++x) {
      tile_coordinates const tc{x, y};
      auto const* const ptile = tile_at(z, tc);

      auto const filename =
          boost::lexical_cast<std::string>(z)
        + '-'
        + boost::lexical_cast<std::string>(tc.y)
        + '-'
        + boost::lexical_cast<std::string>(tc.x)
        + ".png"
      ;

      // Remove in any case, otherwise we may get weird effects
      // by writing to the empty tile inadvertantly
      cpf::unlink(filename, true);
      if (NULL != ptile) {
        //write_tile(*ptile, filename, pix);
      } else {
        // Ignore if file is already deleted
        cpf::link(params.empty_tile_name, filename);
      }

    }} // double for()

  }

  double const elapsed = timer() - start;
  sl << prio::NOTICE << "Flushing tiles: Success, elapsed time [s]: " 
     << elapsed << std::endl;
  } catch (std::exception const& e) {
    sl << prio::ERR << "Flushing tiles failed: " << e.what() << std::endl;
  }
}

/*template<typename T>
void cpl::map::tileset<T>::write_tile(
    cpl::map::tileset<T>::tile_type const& tile,
    std::string const& filename,
    rgba_function const& pix) const {

  png::image<png::rgba_pixel> img(params.tilesize, params.tilesize);

  for (int y = 0; y < params.tilesize; ++y) {
  for (int x = 0; x < params.tilesize; ++x) {
    img[y][x] = pix(tile[x][y]);
  }}

  img.write(filename);
}*/

#endif // CPP_LIB_MAP_H

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

#include "cpp-lib/geodb.h"

#include "cpp-lib/units.h"
#include "cpp-lib/util.h"
#include "cpp-lib/sys/syslogger.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

#include <exception>
#include <istream>
#include <stdexcept>

// using namespace boost::algorithm;
using namespace cpl::util::log;

unsigned type_from_openaip(std::string const& t) {
  if        ("GLIDING" == t 
      || t.find("AF") != std::string::npos 
      || t.find("AD") != std::string::npos) {
    return cpl::gnss::AIRPORT_TYPE_SMALL;
  } else if (t.find("APT") != std::string::npos) {
    return cpl::gnss::AIRPORT_TYPE_LARGE;
  } else if (t.find("HELI") != std::string::npos) {
    return cpl::gnss::AIRPORT_TYPE_HELI;
  } else {
    return 0;
  }
}

    
void cpl::gnss::airport_db_from_openaip(
    cpl::gnss::airport_db& ret,
    std::string const& filename,
    bool const capitalize,
    std::ostream* const sl) {

  if (sl) {
    *sl << prio::NOTICE << "Airport data: Reading from " 
        << filename
        << std::endl;
  }

  boost::property_tree::ptree pt;
  boost::property_tree::read_xml(filename, pt);

  auto const& wp = pt.get_child("OPENAIP.WAYPOINTS");
  for (auto const& elt : wp) {
    cpl::gnss::lat_lon_alt lla;
    airport_db::value_type v;

    try {
    if ("AIRPORT" != elt.first) {
      continue;
    }
    // Location

    auto const& geoloc = elt.second.get_child("GEOLOCATION");
    lla.lat = geoloc.get<double>("LAT");
    lla.lon = geoloc.get<double>("LON");

    double const alt_some_unit = geoloc.get<double>("ELEV");
    auto const& elev_unit = geoloc.get<std::string>("ELEV.<xmlattr>.UNIT");
    if ("M" == elev_unit) {
      lla.alt = alt_some_unit;
    } else if ("FT" == elev_unit) {
      lla.alt = alt_some_unit * cpl::units::foot();
    } else {
      throw std::runtime_error("openaip reader: unknown unit: " + elev_unit);
    }

    // Data
    auto const& name = elt.second.get_optional<std::string>("NAME");
    auto const& icao = elt.second.get_optional<std::string>("ICAO");
    if (name == boost::none && icao == boost::none) {
      if (sl) {
        *sl << prio::WARNING << "Ignoring airport without NAME nor ICAO"
            << std::endl;
      }
      continue;
    }

    if (name != boost::none) { 
      if (capitalize) {
        v.name = cpl::util::utf8_toupper(name.get());
      } else {
        v.name = name.get(); 
      }
    }
    if (icao != boost::none) { 
      cpl::util::verify_alnum(icao.get());
      v.icao = icao.get();
      cpl::util::toupper(v.name);
    }

    auto const& t = elt.second.get<std::string>("<xmlattr>.TYPE");
    v.type = type_from_openaip(t);
    if (0 == v.type) {
      if (sl) {
        *sl << prio::WARNING << "Ignoring airport: " << v.name
            << "; type = " << t
            << std::endl;
      }
      continue;
    }
    } catch (std::exception const& e) {
      if (sl) {
        *sl << prio::WARNING << "Skipping airport due to error: " << e.what()
            << std::endl;
      }
      continue;
    }
    // std::cout << v.type << ','
    //           << v.name << ',' << v.icao << ',' << lla << std::endl;
    ret.add_element(lla, v);
  }
  if (sl) {
    *sl << prio::NOTICE << "Airport data: Read " << ret.size() << " entries"
        << std::endl;
  }
}

void cpl::gnss::airport_db_from_registry(
    cpl::gnss::airport_db& adb,
    cpl::util::registry const& reg,
    std::ostream* const sl) {

  auto const& dir       = reg.check_string("airport_db_directory");
  auto const& countries = reg.check_vector_string("airport_db_country_list");

  cpl::gnss::airport_db_from_openaip(adb, dir, countries, sl);
}

void cpl::gnss::airport_db_from_openaip(
    cpl::gnss::airport_db& adb,
    std::string const& dir,
    std::vector<std::string> const& countries,
    bool const capitalize,
    std::ostream* const sl) {

  for (auto const& c : countries) {
    auto const filename = dir + "/" + c + "_wpt.aip";
    airport_db_from_openaip(adb, filename, capitalize, sl);
  }
}

cpl::gnss::airport_db
cpl::gnss::airport_db_from_csv(
    std::string const& filename,
    std::ostream* const sl) {
  if (sl) {
    *sl << prio::NOTICE << "Airport data: Reading from " 
        << filename
        << std::endl;
  }

  auto is = cpl::util::file::open_read(filename);
  cpl::gnss::airport_db ret;

  std::string line;
  long n = 0;
  while (cpl::util::getline(is, line, 1000)) {
    ++n;
    std::vector<cpl::util::stringpiece> items;
    boost::algorithm::split(items, line, boost::algorithm::is_any_of(","));
    if (items.size() != 5) {
      throw std::runtime_error(
          "airport data from csv: " 
          + boost::lexical_cast<std::string>(line)
          + ": expected 5 fields");

    }
    cpl::gnss::lat_lon_alt lla;
    airport_db::value_type v;
    v.icao.assign(items[0].begin(), items[0].end());

    std::string const t(items[1].begin(), items[1].end());
    if        ("small_airport" == t) {
      v.type = AIRPORT_TYPE_SMALL;
    } else if ("medium_airport" == t || "large_airport" == t) {
      v.type = AIRPORT_TYPE_LARGE;
    } else if ("heliport" == t) {
      v.type = AIRPORT_TYPE_HELI;
    } else {
      if (sl) {
        *sl << prio::WARNING << "Ignoring airport: " << v.icao
            << "; type = " << t
            << std::endl;
      }
      continue;
    }

    if (v.icao.size() > 18) {
      throw std::runtime_error(
          "airport data from csv: " 
          + boost::lexical_cast<std::string>(line)
          + ": icao code/name should have <= 18 characters");
    }

    try {
      // lla.lat = cpl::util::stringpice_cast<double>(items[2]);
      lla.lat = boost::lexical_cast<double>(items[2]);
      lla.lon = boost::lexical_cast<double>(items[3]);
      // Sometimes, the altitude value is missing...
      if (!items[4].empty()) {
        lla.alt = boost::lexical_cast<double>(items[4]) * cpl::units::foot();
      }
    } catch (boost::bad_lexical_cast const& e) {
      throw std::runtime_error(
          "airport data from csv: " 
          + boost::lexical_cast<std::string>(line)
          + ": syntax error: " + e.what());
    }
    ret.add_element(lla, v);
  }

  if (sl) {
    *sl << prio::NOTICE << "Airport data: Read " << ret.size() << " entries"
        << std::endl;
  }
  return ret;
}

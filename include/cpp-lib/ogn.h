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
// Component: AERONAUTICS
//
// * Open Glider Network specific APRS parsing
// * Thermal detection
//
// References:
// - ftp://ftp.tapr.org/aprssig/aprsspec/spec/aprs101/APRS101.pdf
// - http://www.aprs.org/APRS-docs/PROTOCOL.TXT
// - http://www.aprs-dl.de/?APRS_Detailwissen:SSID%2BSymbole
// - http://www.aprs-is.net/q.aspx (qAS, qAC, qAR, ...)
// - http://wiki.glidernet.org/wiki:subscribe-to-ogn-data
// - List of OGN clients: http://glidern1.glidernet.org:14501/
//


#ifndef CPP_LIB_OGN_H
#define CPP_LIB_OGN_H

#include "cpp-lib/gnss.h"
#include "cpp-lib/map.h"
#include "cpp-lib/math-util.h"
#include "cpp-lib/registry.h"
#include "cpp-lib/sys/network.h"

#include <iostream>
#include <thread>
#include <unordered_map>

namespace cpl {

namespace ogn {

// FLARM random hopping, still?!
short constexpr ID_TYPE_RANDOM = 0;
short constexpr ID_TYPE_ICAO   = 1;
short constexpr ID_TYPE_FLARM  = 2;
short constexpr ID_TYPE_OGN    = 3;

// http://www.ediatec.ch/pdf/FLARM_DataportManual_v6.00E.pdf
short constexpr VEHICLE_TYPE_GLIDER      = 1;
short constexpr VEHICLE_TYPE_TOW_PLANE   = 2;
short constexpr VEHICLE_TYPE_HELICOPTER  = 3;
short constexpr VEHICLE_TYPE_PARACHUTE   = 4;
short constexpr VEHICLE_TYPE_DROP_PLANE  = 5;
short constexpr VEHICLE_TYPE_DELTA       = 6;
short constexpr VEHICLE_TYPE_PARAGLIDER  = 7;
short constexpr VEHICLE_TYPE_POWER_PLANE = 8;
short constexpr VEHICLE_TYPE_JET         = 9;
short constexpr VEHICLE_TYPE_UFO         = 10;
short constexpr VEHICLE_TYPE_BALLOON     = 11;
short constexpr VEHICLE_TYPE_AIRSHIP     = 12;
short constexpr VEHICLE_TYPE_UAV         = 13;
// 14 not assigned
short constexpr VEHICLE_TYPE_STATIC      = 15;

// OGN station information:
// - Network name
// - 4D position
struct station_info {
  // Name of network, e.g. GLIDERN1, GLIDERN2.  TODO: Define semantics.
  std::string network;

  // pt.lat, pt.lon, pt.alt, pt.time
  cpl::gnss::position_time pt;

  // CPU (0...1 ?)
  double cpu = 0;

  // RAM [megabyte]
  double ram_used = 0;
  double ram_max = 0;

  // NTP difference (?) [ms]/ppm (?)
  double ntp_difference = 0;
  double ntp_ppm = 0;

  // Temperature [degrees C]
  double temperature = 0;

  // Software version vx.y.z
  std::string version;

  // TODO: RF:+40+2.7ppm/+0.8dB
};

// Aircraft data, valid long-term.  Typically read from DDB or similar
// service.
struct vehicle_data {
  // First name, typically callsign
  std::string name1;

  // Second name, typically competition number (for gliders)
  std::string name2;

  // Vehicle make/model (textual)
  std::string type;
  
  // Track this device, as determined e.g. in the DDB.  If false, 
  // device will not show up on online tracking services.
  bool tracking = true;

  // Identify this device, as determined e.g. in the DDB.  If false, ID 
  // will be randomized, making e.g. SAR difficult.
  bool identify = false;

  // ID type from DDB, one of ID_TYPE_...
  // WARNING: This will often be wrong because users enter the
  // wrong address type in the DB.
  short id_type_probably_wrong = ID_TYPE_RANDOM;

  vehicle_data(
      std::string const& name1,
      std::string const& name2,
      std::string const& type,
      bool const tracking,
      bool const identify)
  : name1(name1),
    name2(name2),
    type(type),
    tracking(tracking),
    identify(identify)
  {}

  vehicle_data()
  : name1(),
    name2(),
    type(),
    tracking(true),
    identify(false)
  {}
};

// Tracker hardware and software version
struct versions {
  std::string hardware = "-";
  std::string software = "-";
};

// Radio signal reception information
struct rx_info {
  rx_info() : rssi(0), frequency_deviation(0) {}
  // Received by (station name)
  std::string received_by;

  // Received signal strength indication [dB]
  double rssi;

  // Frequency deviation [kHz]; TODO: Define sign.
  double frequency_deviation;

  // Bit errors (0e, 1e, field)
  short errors;

  // Is this a relayed packet?
  bool is_relayed = false;
};

// Aircraft reception information:
// - ID type
// - Callsign
// - 3D movement and turn rate
// - 4D position and barometric altitude
// - RX information
struct aircraft_rx_info {
  aircraft_rx_info()
  {}

  // ID type, one of ID_TYPE_...
  short id_type = ID_TYPE_RANDOM;

  // 1-15: FLARM aircraft types
  short vehicle_type = 1;

  // Privacy flags, cf. http://wiki.glidernet.org/opt-in-opt-out
  // Process this device.  Theoretically, no packets should show up
  // which have this set to false.  This is the negation of the FLARM
  // no-track flag as of 2015.  The software discards those packets
  // with a warning.
  bool process = true;

  // Stealth mode: Don't show up on other mobile devices.  Not relevant
  // for online tracking.
  bool stealth = false;

  // HW/SW versions
  versions ver;

  // Callsign, type, tracking flags from DDB
  vehicle_data data;

  // 4D position and accuracy
  cpl::gnss::fix pta;

  cpl::gnss::motion_and_turnrate mot;

  // Barometric altitude above 1013.25 hPa [m]
  float baro_alt = 0;

  // RX info
  rx_info rx;
};

// Parameters for thermal detection from tracked gliders
struct thermal_detector_params {
  thermal_detector_params(int const method);

  thermal_detector_params();

  void validate();

  // Method.  0 == don't detect,
  // 1 = single point, 2 = two points (potential altitude difference)
  int method = 2;

  // Thermal marker size [pixels]
  int dot_size = 1;

  // Maximum time delta between two points to consider for PA difference
  // method [s]
  double max_time_delta = 12;

  // Only consider gliders slower than this speed [m/s].
  // That would include most paragliders and hang gliders
  double max_speed = 30;

  // Minimum turn rate to detect a thermal [degrees/s], only for gliders
  double min_turnrate_glider = 6;

  // Minimum climb rate to consider [m/s]
  double min_climbrate = 0.5;
};

thermal_detector_params thermal_detector_params_from_registry(
    cpl::util::registry const&,
    thermal_detector_params const& defaults = thermal_detector_params{});

struct thermal {
  // Position of thermal, time of measurement
  cpl::gnss::position_time pt;

  // Measured aircraft climb rate [m/s]
  double climbrate;
};

// Small footprint aggregator struct for use in tiles
struct thermal_aggregator {
  // 3 bits for validity, translating into alpha
  // 0 ... no info
  // 7 ... recent info
  unsigned char validity : 3;

  // 5 bits for thermal strength
  // 0  ... 0   m/s
  // 31 ... 6.2 m/s
  unsigned char strength : 5;

  thermal_aggregator()
  : validity(0),
    strength(0)
  {}

  double climbrate() const {
    return strength * 0.2;
  }

  double alpha() const {
    return validity / 7.0;
  }
};

inline std::ostream& operator<<(std::ostream& os, thermal_aggregator const ag) {
  return os << ag.climbrate();
}

inline void update_thermal_aggregator(
    thermal_aggregator& ag, 
    thermal const& th) {
  double const a = ag.validity / 7.0;
  double const b = 1 - a;

  double const th_strength = th.climbrate * 5;
  double new_strength = a * ag.strength + b * th_strength;
  cpl::math::clamp(new_strength, 0.0, 31.9999);

  ag.validity = 7;
  ag.strength = new_strength;
}

// Age aggregator, return true iff it's still > 0 after the update.
inline bool age_aggregator(thermal_aggregator& ag) {
  if (ag.validity >= 2) {
    --ag.validity;
    return true;
  }
  if (1 == ag.validity) {
    --ag.validity;
  }
  return false;
}

typedef cpl::map::tileset<thermal_aggregator> thermal_tileset;

// Updates a thermal tileset with a given thermal
void update(thermal_detector_params const&, thermal_tileset&, thermal const&);

// Returns a hash-comment line with the thermal output format
char const* thermal_format_comment();

// Detect a thermal based on RX info.  If a thermal is detected,
// the position of the returned thermal is valid, invalid otherwise.
thermal detect_thermal(
    thermal_detector_params const&,
    aircraft_rx_info const&);

// Detect a thermal based on current and previous RX info.  Works
// by comparing potential altitudes.  Potential altitude
// PA = A + v^2 / 2g, where v includes vertical speed.
// If params.method == 1 or previous is NULL, uses the one-parameter method
// (above).
thermal detect_thermal(
    thermal_detector_params const& params,
    aircraft_rx_info const& current,
    aircraft_rx_info const* previous);

// Getters
inline std::string const& callsign(aircraft_rx_info const& info) {
  return info.data.name1;
}

// Station info and name
typedef std::pair<std::string, station_info> station_info_and_name;

// Vehicle data and *unqualified* ID, i.e. just the
// 6 hex digits
typedef std::pair<std::string, vehicle_data> vehicle_data_and_name;

// Aircraft database
typedef std::map<std::string, aircraft_rx_info> aircraft_db;

// Complete aircraft RX info and ID
typedef std::pair<std::string, aircraft_rx_info> aircraft_rx_info_and_name;

// Vehicle database type
typedef std::unordered_map<std::string, vehicle_data> vehicle_db;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

// The default host/service (port) to connect to OGN.
inline std::string default_host    () { return "aprs.glidernet.org"; }
// Email from Sebastien March 10, 2015: 10152 doesn't require a filter
inline std::string default_service () { return "10152";              }
inline std::string default_username() { return "0";                  }

inline std::string default_ddb_url() {
  return "http://ddb.glidernet.org/download/";
}

// Query DDB every 600 seconds
inline double default_ddb_query_interval() {
  return 600;
}

// Returns flarm:<id>, icao:<id> etc. depending on id_type
std::string qualified_id(std::string const& id, short id_type);

// Returns <id> for flarm:<id>, etc.
std::string unqualified_id(std::string const& id);

// Replaces the last n digits of <id> by <replacement>.  
// E.g., returns e.g. flarm:DF0000 if passed flarm:DF48A3.
// n must be >= 0.
std::string hide_id(std::string const& id, int n = 4, char replacement = '0');

// Connects to the given OGN host and returns the connection.
// Logs to log.
std::unique_ptr<cpl::util::network::connection> connect(
    std::ostream& log,
    const std::string& host = default_host(),
    const std::string& service = default_service());

// Logs into OGN with the given login information (version) and
// filter.
// Uses os as the connection to OGN and logs to log.
// Uses is as the stream from OGN to us. 
// Checks and logs the reply in is.
// Filter syntax: r/lat/lon/range, e.g. r/47/8/500
// Range unit: ?
// No filter for worldwide access.
void login(
    std::ostream& log,
    std::ostream& os,
    std::istream& is,
    const std::string& version,
    const std::string& filter = "",
    const std::string& username = default_username());

// Gets the vehicle database from OGN DDB with optional URL.
// Logs to log.
// Doesn't throw, but logs errors to log.  Returns an empty DB on errors.
// If URL doesn't start with "http", reads a local file
vehicle_db get_vehicle_database_ddb(
    std::ostream& log,
    std::string const& url = default_ddb_url());

struct ddb_handler {

// Parses an APRS line containing aircraft info and stores data in acft,
// converting units as appropriate.
// Returns true on success.
//
// Example format:
// ICA3D28CB>APRS,qAS,EDMC:/175426h4829.84N/01014.30E'353/122/A=002467 id053D28CB -078fpm +0.4rot 6.0dB 0e +2.3kHz gps2x2
//
// If utc is given and >= 0, it must represent the UTC seconds since
// 00:00 January 1, 1970.  In this case, the date implied in utc
// is attached to the (time-only) HHMMSS field from the APRS packet
// to compute acft.second.pta.time.  If utc < 0, the date in
// acft.second.pta.time will be January 1st, 1970.  
bool parse_aprs_aircraft(
    std::string const& line, 
    aircraft_rx_info_and_name& acft,
    double const utc = -1);

  // Instantiate the parser, logging to log (only used during construction).
  // If query_interval > 0 [s], starts a background thread that queries
  // the OGN DDB at regular intervals and uses it to determine
  // callsigns, tracking flags etc.
  // If initial_vdb is given and nonempty, reads a vehicle DB from
  // the given file/URL on startup.
  ddb_handler(std::ostream& log,
              double query_interval = default_ddb_query_interval(),
              std::string const& initial_vdb = "");

  // Force replacement of vehicle DB if the given DB is nonempty.
  void set_vdb(std::ostream& log, cpl::ogn::vehicle_db&& new_db);

  // Join thread...
  ~ddb_handler();

  // If we have a DDB, apply it to the given aircraft record,
  // i.e. set its callsign from the ID.
  void apply(aircraft_rx_info_and_name&);

private:
  double query_interval;
  bool query_thread_active;
  bool has_nontrivial_vdb;
  vehicle_db vdb;
  std::mutex vdb_mutex;

  std::thread query_thread;

  void query_thread_function();
};

// For compatibility with earlier API version
using aprs_parser = ddb_handler;
  
// Parses an APRS line containing receiver station info 
// and stores data in stat.
// Returns true on success.
//
// Example format:
// LFLO>APRS,TCPIP*,qAC,GLIDERN2:/175435h4603.32NI00359.99E&/A=001020 CPU:0.6 RAM:340.6/492.2MB NTP:0.6ms/-30.5ppm +67.0C RF:+46-1.2ppm/+0.3dB
// See parse_aprs_aircraft() for the utc parameter.
// TODO: Only station name, position, time and NTP are currently parsed.
bool parse_aprs_station(
    std::string const& line, 
    station_info_and_name& stat,
    double const utc = -1);


////////////////////////////////////////////////////////////////////////
// std::ostream output
////////////////////////////////////////////////////////////////////////
// Write data, format: network time lat lon alt
std::ostream& operator<<(std::ostream&, station_info const&);

// Write data, format: id_type callsign time lat lon alt course speed vspeed
// turn_rate rx_info
std::ostream& operator<<(std::ostream&, aircraft_rx_info const&);

// Write data, format: received_by rssi frequency_deviation
std::ostream& operator<<(std::ostream&, rx_info const&);

// Write data, format: time lat lon alt climbrate
std::ostream& operator<<(std::ostream&, thermal const&);

////////////////////////////////////////////////////////////////////////
// Testing
////////////////////////////////////////////////////////////////////////
void unittests(std::ostream&);

} // namespace ogn

} // namespace cpl


#endif // CPP_LIB_OGN_H

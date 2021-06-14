/**
 * @file
 * @brief Implementation of methods for the movement trajectory analysis.
 * @details
 * The DataRow is a data structure used for holding data logs.
 */
#include "general_functions.h"
#include "nlohmann/json.hpp"    // used for construct geojson

using json = nlohmann::json;

class DataRow {
private:
  tm datetime_;
  double lon_;
  double lat_;
  std::string tag_;
  int areaId_;

public:
  DataRow(tm datetime, double lon, double lat, std::string tag) {
    datetime_ = datetime;
    lon_ = lon;
    lat_ = lat;
    tag_ = tag;
    areaId_ = 0;
  }
  double getLon() { return lon_; }
  double getLat() { return lat_; }
  tm getDateTime() { return datetime_; }
  std::string getTag() { return tag_; }
  void setAreaID(int id) { areaId_ = id; }
  int getAreaID() { return areaId_; }
};

struct compareByTime {
  bool operator()(DataRow & a, DataRow & b) {
    return difftime(getTimeValue(a.getDateTime()), getTimeValue(b.getDateTime())) < 0;
  }
};

void createJsonFile(std::string filename, std::vector<DataRow>& list, int low, int high) {
  std::ofstream ofsMap(filename);
  json map;
  map["type"] = "MultiPoint";
  map["coordinates"] = {};
  for (int i = low; i < high; i++) {
    map["coordinates"] += {list[i].getLon(), list[i].getLat()};
  }
  ofsMap << map.dump(4);  // format(4) is easy to read
  ofsMap.close();
}

// reference: https://stackoverflow.com/questions/6671183/calculate-the-center-point-of-multiple-latitude-longitude-coordinate-pairs
std::vector<double> centerOfGravity (std::vector<DataRow> &list, int areaID) {
  std::vector<double> midpoints(2); //Lat, Lon
  std::cout << "\nMethod: Center of gravity" << std::endl;
  std::cout << "Area " << std::to_string(areaID) << std::endl;
  double count = 0;
  float cart_x = 0, cart_y = 0, cart_z = 0;
  for (DataRow &d : list) {
    if (d.getAreaID() == areaID) {
      count++;
      cart_x += cos(deg2rad(d.getLat())) * cos(deg2rad(d.getLon()));
      cart_y += cos(deg2rad(d.getLat())) * sin(deg2rad(d.getLon()));
      cart_z += sin(deg2rad(d.getLat()));
    }
  }
  cart_x /= count;
  cart_y /= count;
  cart_z /= count;
  midpoints[0] = rad2deg(atan2(cart_z, sqrt(pow(cart_x, 2) + pow(cart_y, 2))));
  midpoints[1] = rad2deg(atan2(cart_y, cart_x));
  std::cout.precision(10);
  std::cout << "Midpoint: " << midpoints[0] << ", " << midpoints[1] << std::endl;
  
  return midpoints;
}

std::vector<double> averageLatLon (std::vector<DataRow> &list, int areaID) {
  std::vector<double> midpoints(2); //Lat, Lon
  std::cout << "\nMethod: Average latitude/longitude" << std::endl;
  std::cout << "Area " << std::to_string(areaID) << std::endl;
  double sumLon = 0, sumLat = 0;
  int count = 0;
  for (DataRow d : list) {
    if (d.getAreaID() == areaID) {
      sumLon += d.getLon();
      sumLat += d.getLat();
      count++;
    }
  }
  midpoints[0] = sumLat / count;
  midpoints[1] = sumLon / count;
  std::cout.precision(10);
  std::cout << "Midpoint: " << midpoints[0] << ", " << midpoints[1] << std::endl;

  return midpoints;
}

void midpointAnalysis(std::vector<DataRow> &list, int areaCount, bool useAverage) {
  std::string method = "gravity";
  if (useAverage) method = "average";
  for (int i = 1; i <= areaCount; i++) {
    std::vector<double> midpoints (2, 0);
    if (useAverage) midpoints = averageLatLon(list, i);
    else midpoints = centerOfGravity(list, i);
    double count = 0;
    double meanLat = midpoints[0], meanLon = midpoints[1];

    // Center of minimum distance from web calculator
    // if (i == 1) {
    //   meanLat = 25.045682;
    //   meanLon = 121.512526;
    // } else if (i == 2) {
    //   meanLat = 25.050978;
    //   meanLon = 121.299258;
    // }

    // output the file for plotting
    double diffSum = 0, diffMax = 0, diffMin = 1;
    std::ofstream ofsMid(method + "-area-" + std::to_string(i) + ".csv");
    for (DataRow d : list) {
      if (d.getAreaID() == i) {
        count++;
        double diff = distanceEarth(meanLat, meanLon, d.getLat(), d.getLon());
        diffSum += diff;
        diffMax = fmax(diffMax, diff);
        diffMin = fmin(diffMin, diff);
      }
    }
    std::cout << "\taverage difference: " << diffSum / count << std::endl;
    std::cout << "\tmaximum difference: " << diffMax << std::endl;
    std::cout << "\tminimum difference: " << diffMin << std::endl;
    
    // for CDF plot
    if (i == 1) diffMax = 0.7;  // maxdiff of area 1
    else if (i == 2) diffMax = 0.4; // maxdiff of area 2
    double numSample = 50;
    for (int j = 1; j <= numSample; j++) {
      double bound = diffMax * j / numSample;
      ofsMid << bound << ",";
      int lowerCount = 0;
      for (DataRow d : list) {
        if (d.getAreaID() == i) {
          double diff = distanceEarth(meanLat, meanLon, d.getLat(), d.getLon());
          if (diff <= bound) lowerCount++;
        }
      }
      ofsMid << 100 * lowerCount / count << std::endl;
    }

    ofsMid.close();
  }
}

// generate inputs of a web calculator http://www.geomidpoint.com/
void generateGeoFiles(std::vector<DataRow> &list, int areaCount) {
  for (int i = 1; i <= areaCount; i++) {
    std::ofstream ofsLon("area-" + std::to_string(i) + "-lon.txt");
    std::ofstream ofsLat("area-" + std::to_string(i) + "-lat.txt");
    for (DataRow d : list) {
      if (d.getAreaID() == i) {
        ofsLon << d.getLon() << std::endl;
        ofsLat << d.getLat() << std::endl;
      }
    }
    ofsLon.close();
    ofsLat.close();
  }
}

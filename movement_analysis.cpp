/**
 * @file
 * @brief Implementation of methods for the movement trajectory analysis.
 * @details
 * The DataRow is a data structure used for holding data logs.
 * The Cell is a data structure used for holding data logs associated with a cell.
 * The User is a data structure used for holding data logs associated with a user.
 * ### Key functions in User
 * 1. numConnections: Given a cell, output the number of connections logged.
 *
 * 2. getTimeSegments: Given a cell and an interval, output the set of time segements within the interval.
 * 
 * 3. findResidentialAreaByCell: Output json files of possible residential areas by finding cells with the top k largest numConnections.
 * 
 * 4. findResidentialAreaBySpeed: Output json files of possible residential areas by user movement detection.
 */
#include <ctime>
#include <unordered_map>
#include <queue>
#include "csv_parser.h"         // used for csv parsing
#include "haversine_formula.h"  // used for calculating the great-circle distance
#include "nlohmann/json.hpp"    // used for construct geojson

using json = nlohmann::json;

typedef std::pair<std::string, int> PAIR;

struct compareBySecondValue {
  bool operator()(const PAIR & a, const PAIR & b) {
    return a.second < b.second;
  }
};

class DataRow {
private:
  tm datetime_;
  double lon_;
  double lat_;
  std::string tag_;

public:
  DataRow(tm datetime, double lon, double lat, std::string tag) {
    datetime_ = datetime;
    lon_ = lon;
    lat_ = lat;
    tag_ = tag;
  };
  time_t getTimeValue() {
    time_t t = mktime(&datetime_); 
    if (t == -1) {
      std::cout << "The date couldn't be converted." << std::endl;
      exit(0);
    }
    return t;
  };
  std::string getTimeString(bool useColon) {
    char buffer[50];
    if (useColon) strftime(buffer, sizeof(buffer), "%T", &datetime_);
    else strftime(buffer, sizeof(buffer), "%H%M%S", &datetime_);
    return buffer;
  }
  double getLon() { return lon_; };
  double getLat() { return lat_; };
};

struct compareByTime {
  bool operator()(DataRow & a, DataRow & b) {
    return difftime(a.getTimeValue(), b.getTimeValue()) < 0;
  }
};

class Cell {
private:  
  std::string tag_;
  std::vector<DataRow> rowList_;

public:
  Cell(DataRow r, std::string tag) {
    rowList_.push_back(r);
    tag_ = tag;
  };
  void addDataRow(DataRow d) { rowList_.push_back(d); };
  int numConnections() { return rowList_.size(); };
  bool isWithinInterval(int i, int j, int interval);
  std::vector<std::string> getTimeSegments(int interval);
  std::string getName() { return tag_; };
  std::vector<DataRow>& getRowList() { return rowList_; };
};

class User {
private:
  std::vector<DataRow> rowList_;
  std::unordered_map<std::string, int> cellMap_; // map cell tag to its index in cellList_
  std::vector<Cell> cellList_;

  // used for finding cells with top k largest numConnections
  std::priority_queue<PAIR, std::vector<PAIR>, compareBySecondValue> cellQueue_;

public:
  User(std::string filename) { readFile(filename); };
  void readFile(std::string filename);
  int numConnections(std::string cell) {
    isValid(cell);
    return cellList_[cellMap_[cell]].numConnections();
  };
  std::vector<std::string> getTimeSegments(std::string cell, int interval) {
    isValid(cell);
    return cellList_[cellMap_[cell]].getTimeSegments(interval);
  };
  void findResidentialAreaByCell(int k, int interval);
  void findResidentialAreaBySpeed();
  void isValid(std::string cell) { 
    if(cellMap_.count(cell) == 0) {
      std::cout << "ERROR: This cell does not exist." << std::endl;
      exit(0);
    } 
  };
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

/**
 * @returns the boolean value indicating if the time interval between DataRow i and DataRow j is less than or equal to the specific interval.
 */
bool Cell::isWithinInterval(int i, int j, int interval) {
  if (i < 0 || j < 0 || i >= rowList_.size() || j >= rowList_.size()) {
    std::cout << "ERROR: Out of range (rowList_)." << std::endl;
    exit(0);
  }
  if (interval < 0) {
    std::cout << "ERROR: Invalid interval." << std::endl;
    exit(0);
  }
  return difftime(rowList_[j].getTimeValue(), rowList_[i].getTimeValue()) <= interval;
}

std::vector<std::string> Cell::getTimeSegments(int interval) {
  sort(rowList_.begin(), rowList_.end(), compareByTime());
  std::vector<std::string> segmentList;
  std::string segment = "";
  int low = 0, high = 0;

  for (int i = 0; i < rowList_.size(); i++) {
    if (segment == "") {
      low = i;
      segment += rowList_[i].getTimeString(1);
    }

    if (!isWithinInterval(low, i, interval)) {
      segment += "-to-" + rowList_[high].getTimeString(1);
      segmentList.push_back(segment);
      segment = rowList_[i].getTimeString(1);;
      low = i;
    }
    high = i;
  }
  segment += "-to-" + rowList_[high].getTimeString(1);
  segmentList.push_back(segment);

  for (std::string s : segmentList) {
    std::cout << s << std::endl;
  }
  return segmentList;
}

void User::readFile(std::string filename) {
  std::ifstream dataSource(filename);
  if (!dataSource) {
    std::cout << "ERROR: The file cannot be opened." << std::endl;
    exit(0);
  }

  CSVRow row;
  dataSource >> row; // skip the first line
  while (dataSource >> row) {
    tm tm = {};
    std::stringstream ss(row[0]);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    DataRow d(tm, stod(row[1]), stod(row[2]), row[3]);
    rowList_.push_back(d);

    if (cellMap_.count(row[3]) > 0) {
      int idx = cellMap_[row[3]];
      cellList_[idx].addDataRow(d);
    } else {
      Cell c(d, row[3]);
      cellList_.push_back(c);
      cellMap_[row[3]] = cellList_.size() - 1;
    }
  }

  for (auto c : cellList_) {
    cellQueue_.push({c.getName(), c.numConnections()});
  }

  dataSource.close();
}

/**
 * For each selected cell, output all locations logged associated with it.
 * @returns the geojson files.
 */
void User::findResidentialAreaByCell(int k, int interval) {
  for (int i = 1; i <= k; i++) {
    std::string cellTag = cellQueue_.top().first;
    int num = cellQueue_.top().second;
    std::cout << "\nTop" << i << ": ";
    std::cout << cellTag << ", Num:" << cellQueue_.top().second << std::endl;
    getTimeSegments(cellTag, interval);
    
    int index = cellMap_[cellTag];
    std::string mapFile = "map-by-cell-top-" + std::to_string(i) + ".json";
    createJsonFile(mapFile, cellList_[index].getRowList(), 0, num);
    
    cellQueue_.pop();
  }
}

/**
 * Methodology:
 * 1. Scan the sorted data and divide the whole data into several segments.
 * 2. Compute the speed of moving from the previos location to the current location.
 * 3. Cut data if the speed exceed a constant (e.g., general human speed).
 * 4. Only segments with a specific time interval are selected.
 * @returns the geojson files of each selected segment.
 */
#define movingSpeed 0.0125  // Human speed: 45 km per hour = 0.0125 km per second
#define upscalingFactor 1.1 // Upscale the distance between two locations because the distance is an airline distance.
#define minInterval 600     // seconds

void User::findResidentialAreaBySpeed() {
  sort(rowList_.begin(), rowList_.end(), compareByTime());
  int mapID = 1;
  int low = 0, high = 0;
  double stayInterval = 0;
  for (int i = 1; i < rowList_.size(); i++) {
    high = i;
    double currShift = distanceEarth(
      rowList_[i - 1].getLat(), rowList_[i - 1].getLon(),
      rowList_[i].getLat(), rowList_[i].getLon()
      );
    double timeDiff = difftime(rowList_[i].getTimeValue(), rowList_[i - 1].getTimeValue());
    if (timeDiff < 0) {
      std::cout << "ERROR: timeDiff < 0. " << timeDiff << std::endl;
      exit(0);
    }
    if (currShift == 0 || timeDiff == 0) continue;

    double speed = currShift * upscalingFactor / timeDiff;
    if (speed > movingSpeed) {
      stayInterval = difftime(rowList_[high - 1].getTimeValue(), rowList_[low].getTimeValue());
      if (stayInterval > minInterval) {
        std::string mapFile = "map-by-speed-" + std::to_string(mapID++) + "-" + 
                              rowList_[low].getTimeString(0) + "-to-" + 
                              rowList_[high - 1].getTimeString(0) + ".json";
        createJsonFile(mapFile, rowList_, low, high);
      }
      low = i;
    }
  }
  
  // output the last segment
  high++;
  stayInterval = difftime(rowList_[high - 1].getTimeValue(), rowList_[low].getTimeValue());
  if (stayInterval > minInterval) {
    std::string mapFile = "map-by-speed-" + std::to_string(mapID++) + "-" + 
                          rowList_[low].getTimeString(0) + "-to-" + 
                          rowList_[high - 1].getTimeString(0) + ".json";
    createJsonFile(mapFile, rowList_, low, high);
  }
}

/**
 * Main function:
 * Declare a user and analyse its data.
 * @returns 0 on exit
 */
int main() {
  std::string dataFile = "data.csv";
  double interval = 3600; // seconds
  User u(dataFile);
  std::string targetCell = "CELL_133";
  std::cout << u.numConnections(targetCell) << std::endl;
  u.getTimeSegments(targetCell, interval);

  int k = 10;
  u.findResidentialAreaByCell(k, interval);
  u.findResidentialAreaBySpeed();

  return 0;
}
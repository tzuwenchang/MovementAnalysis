/**
 * @file
 * @brief Implementation of methods for the movement trajectory analysis.
 * @details
 * The User is a data structure used for holding data logs associated with a user.
 * ### Key functions in User
 * 1. numConnections: Given a cell, output the number of connections logged.
 *
 * 2. getTimeSegments: Given a cell and an interval, output the set of time segements within the interval.
 * 
 * 3. findResidentialAreaByTopKCells: Find residential areas by finding cells with the top k largest numConnections.
 * 
 * 4. findResidentialAreaBySpeed: Output json files of possible residential areas by user movement detection.
 */

#include "cell.h"
#include <queue>

typedef std::pair<std::string, int> PAIR;

struct compareBySecondValue {
  bool operator()(const PAIR & a, const PAIR & b) {
    return a.second < b.second;
  }
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
  void findResidentialAreaByTopKCells(int interval);
  void findResidentialAreaBySpeed();
  void calculateSpeedOfEachTime();
  int numConnections(std::string cell) {
    isValid(cell);
    return cellList_[cellMap_[cell]].numConnections();
  };
  std::vector<TIMEPAIR> getTimeSegments(std::string cell, int interval) {
    isValid(cell);
    return cellList_[cellMap_[cell]].getTimeSegments(interval);
  };
  void isValid(std::string cell) { 
    if(cellMap_.count(cell) == 0) {
      std::cout << "ERROR: This cell does not exist." << std::endl;
      exit(0);
    } 
  };
};

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

  for (Cell &c : cellList_) {
    cellQueue_.push({c.getName(), c.numConnections()});
    sort(c.getRowList().begin(), c.getRowList().end(), compareByTime());
  }
  dataSource.close();
  sort(rowList_.begin(), rowList_.end(), compareByTime());
}

/**
 * Methodology:
 * 1. Iterate Top K Cells.
 * 2. For each selected cell, calculate the maximum stay time T. The algorithm terminates when T < a constant time.
 * 3. For each selected cell, find time segments and calculate the stay time t.
 * 4. A cell is in a residential area if t > a constant time.
 * 5. Determine whether the discovered residential area A is new or not by checking if A can be merged to an existing residential area.
 * @returns the geojson files of each selected segment.
 */
void User::findResidentialAreaByTopKCells(int interval) {
  std::unordered_map<std::string, int> areaMap; // used to update areaID in each datarow
  int areaID = 1;
  int topIdx = 1;
  std::vector<std::vector<TIMEPAIR> > areaList; // used to store merged segments of each area
  while (!cellQueue_.empty()) {
    std::string cellTag = cellQueue_.top().first;
    int num = cellQueue_.top().second;
    // std::cout << "\nTop" << topIdx++ << ": ";
    // std::cout << cellTag << ", Num:" << cellQueue_.top().second << std::endl;
    std::vector<TIMEPAIR> currSegList = cellList_[cellMap_[cellTag]].getTimeSegments(interval);
    
    // break when numConnections of this cell is too small (i.e., the stayTime cannot be greater than 3600s)
    if (num < 3600 / interval) break;

    int stayTime = currSegList.size() * interval;
    // std::cout << "stay time: " << stayTime << std::endl;
    if(stayTime > 3600) { // > 1 hr
      bool merged = false;
      for(int i = 0; i < areaList.size(); i++) {
        std::vector<TIMEPAIR> mergedSegList = merge(currSegList, areaList[i]);    
        // some segments are overlapped if some segments are merged
        if (mergedSegList.size() < currSegList.size() + areaList[i].size()) { 
          areaList[i] = mergedSegList;
          areaMap[cellTag] = i + 1; // areaID = index + 1
          merged = true;
          break;
        }
      }
      // this area is new
      if (!merged) {
        areaMap[cellTag] = areaID++;
        areaList.push_back(currSegList);
      }
    }
    cellQueue_.pop();
  }

  std::ofstream ofsArea("time-vs-area.csv"); // output the file for plotting
  ofsArea << "time,areaID" << std::endl;
  // update areaID of each datarow and output 
  for (auto &r : rowList_) {
    if (areaMap.count(r.getTag()) > 0)
      r.setAreaID(areaMap[r.getTag()]);
    ofsArea << getTimeString(r.getDateTime(), 1) << "," << r.getAreaID() << std::endl;
  }
  ofsArea.close();

  midpointAnalysis(rowList_, areaID - 1, false);  // Center of Gravity
  midpointAnalysis(rowList_, areaID - 1, true); // Average
  generateGeoFiles(rowList_, areaID - 1); // for calculating center of minimum distance via web http://www.geomidpoint.com/
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
  int mapID = 1;
  int low = 0, high = 0;
  double stayInterval = 0;
  for (int i = 1; i < rowList_.size(); i++) {
    high = i;
    double currShift = distanceEarth(
      rowList_[i - 1].getLat(), rowList_[i - 1].getLon(),
      rowList_[i].getLat(), rowList_[i].getLon()
      );
    double timeDiff = difftime(getTimeValue(rowList_[i].getDateTime()), getTimeValue(rowList_[i - 1].getDateTime()));
    if (timeDiff < 0) {
      std::cout << "ERROR: timeDiff < 0. " << timeDiff << std::endl;
      exit(0);
    }
    if (currShift == 0 || timeDiff == 0) continue;

    double speed = currShift * upscalingFactor / timeDiff;
    if (speed > movingSpeed) {
      stayInterval = difftime(getTimeValue(rowList_[high - 1].getDateTime()), getTimeValue(rowList_[low].getDateTime()));
      if (stayInterval > minInterval) {
        std::string mapFile = "map-by-speed-" + std::to_string(mapID++) + "-" + 
                              getTimeString(rowList_[low].getDateTime(), 0) + "-to-" + 
                              getTimeString(rowList_[high - 1].getDateTime(), 0) + ".json";
        createJsonFile(mapFile, rowList_, low, high);
      }
      low = i;
    }
  }
  
  // output the last segment
  high++;
  stayInterval = difftime(getTimeValue(rowList_[high - 1].getDateTime()), getTimeValue(rowList_[low].getDateTime()));
  if (stayInterval > minInterval) {
    std::string mapFile = "map-by-speed-" + std::to_string(mapID++) + "-" + 
                          getTimeString(rowList_[low].getDateTime(), 0) + "-to-" + 
                          getTimeString(rowList_[high - 1].getDateTime(), 0) + ".json";
    createJsonFile(mapFile, rowList_, low, high);
  }
}

void User::calculateSpeedOfEachTime() {
  std::ofstream ofsSpeed("time-vs-speed.csv");
  ofsSpeed << "time,speed" << std::endl;
  for (int i = 1; i < rowList_.size(); i++) {
    double currShift = distanceEarth(
      rowList_[i - 1].getLat(), rowList_[i - 1].getLon(),
      rowList_[i].getLat(), rowList_[i].getLon());
    
    double timeDiff = difftime(getTimeValue(rowList_[i].getDateTime()), getTimeValue(rowList_[i - 1].getDateTime()));
    
    if (timeDiff < 0) {
      std::cout << "ERROR: timeDiff < 0. " << timeDiff << std::endl;
      exit(0);
    }
    if (timeDiff == 0) continue;
    
    double speed = 3600 * currShift / timeDiff; // km per hour
    ofsSpeed << getTimeString(rowList_[i].getDateTime(), 1) << "," << speed << std::endl;
  }
  ofsSpeed.close();
}

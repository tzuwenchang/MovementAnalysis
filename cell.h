/**
 * @file
 * @brief Implementation of methods for the movement trajectory analysis.
 * @details
 * The Cell is a data structure used for holding data logs associated with a cell.
 */
#include "datarow.h"

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
  std::vector<TIMEPAIR> getTimeSegments(int interval);
  std::string getName() { return tag_; };
  std::vector<DataRow>& getRowList() { return rowList_; };
};

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
  return difftime(getTimeValue(rowList_[j].getDateTime()), getTimeValue(rowList_[i].getDateTime())) <= interval;
}

std::vector<TIMEPAIR> Cell::getTimeSegments(int interval) {
  std::vector<TIMEPAIR> segmentList;
  TIMEPAIR segment;
  int low = 0, high = 0;
  segment.first = rowList_[0].getDateTime();
  for (int i = 0; i < rowList_.size(); i++) {
    if (!isWithinInterval(low, i, interval)) {
      segment.second = rowList_[high].getDateTime();
      segmentList.push_back(segment);
      segment.first = rowList_[i].getDateTime();
      low = i;
    }
    high = i;
  }
  segment.second = rowList_[high].getDateTime();
  segmentList.push_back(segment);

  // for (TIMEPAIR tp : segmentList) {
  //   std::cout << getTimeString(tp.first, 0) << "-to-" << getTimeString(tp.second, 0) << std::endl;
  // }
  return segmentList;
}
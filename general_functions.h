#include <ctime>
#include <string>
#include <iostream>

typedef std::pair<tm, tm> TIMEPAIR;

std::string getTimeString(tm datetime, bool useColon) {
  char buffer[50];
  if (useColon) strftime(buffer, sizeof(buffer), "%T", &datetime);
  else strftime(buffer, sizeof(buffer), "%H%M%S", &datetime);
  return buffer;
}

time_t getTimeValue(tm datetime) {
  time_t t = mktime(&datetime); 
  if (t == -1) {
    std::cout << "The date couldn't be converted." << std::endl;
    exit(0);
  }
  return t;
};

std::vector<TIMEPAIR> merge(std::vector<TIMEPAIR> v1, std::vector<TIMEPAIR> v2) {
  std::vector<TIMEPAIR> merged;
  std::vector<TIMEPAIR> *target = &v1;
  int idx1 = 0, idx2 = 0, idxTarget = 0;
  while (idx1 <= v1.size() && idx2 <= v2.size()) {
    if (idx1 < v1.size() && idx2 < v2.size()) {
      if (getTimeValue(v1[idx1].first) < getTimeValue(v2[idx2].first)) {
        target = &v1;
        idxTarget = idx1++;
      } else {
        target = &v2;
        idxTarget = idx2++;
      }
    }
    else if (idx1 == v1.size()) {
      idx1++;
      target = &v2;
      idxTarget = idx2++;
    } 
    else if (idx2 == v2.size()) {
      idx2++;
      target = &v1;
      idxTarget = idx1++;
    }

    if (merged.empty() || getTimeValue(merged.back().second) < getTimeValue(target->at(idxTarget).first)) {
      merged.push_back(target->at(idxTarget));
    } else {
      if (getTimeValue(merged.back().second) < getTimeValue(target->at(idxTarget).second))
        merged.back().second = target->at(idxTarget).second;
    }
  }
  
  if (idx1 < v1.size()) merged.insert(merged.end(), v1.begin() + idx1, v1.end());
  if (idx2 < v2.size()) merged.insert(merged.end(), v2.begin() + idx2, v2.end());

  return merged;
}


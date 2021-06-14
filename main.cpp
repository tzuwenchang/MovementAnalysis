#include "csv_parser.h"         // used for csv parsing
#include "haversine_formula.h"  // used for calculating the great-circle distance
#include "user.h"

/**
 * Main function:
 * Declare a user and analyse its data.
 * @returns 0 on exit
 */
int main() {
  std::string dataFile = "data.csv";
  double interval = 180; // seconds
  User u(dataFile);
  std::string targetCell = "CELL_133";
  // std::cout << u.numConnections(targetCell) << std::endl;
  // u.getTimeSegments(targetCell, interval);

  u.findResidentialAreaByTopKCells(interval);
  u.calculateSpeedOfEachTime();
  // u.findResidentialAreaBySpeed();

  return 0;
}
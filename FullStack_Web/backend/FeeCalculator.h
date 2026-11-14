#ifndef FEE_CALCULATOR_H
#define FEE_CALCULATOR_H
#include <string>
using namespace std;
double calculateParkingFee(const string& vehicleType,int hours);
double hourlyParkingRate(const string& vehicleType);
#endif

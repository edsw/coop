//http://stjarnhimlen.se/comp/sunriset.c
#ifndef Sol_Arduino_h
#define Sol_Arduino_h

#include <math.h>
#include <DS3231_Simple.h>
#include "CoopTypes.cpp"

class Sol
{
public:
    Sol();
    Sol(Location loc, LocalTimeParams ltp);
    DateTime Sunrise;
    DateTime Sunset;
    void PrintTo(Stream &Printer);
    
private:
    Location _loc;
    LocalTimeParams _ltp;
    boolean _initialized = false;
    DateTime dt_from_double(double d);
    int days_since_2000_Jan_0(int y, int m, int d);
    double sind(double x);
    double cosd(double x);
    double tand(double x);
    double atand(double x);
    double asind(double x);
    double acosd(double x);
    double atan2d(double y, double x);
    void sunriset();
    double revolution(double x);
    double rev180(double x);
    double GMST0(double d);
    void sun_RA_dec(double d, double *RA, double *dec, double *r);
    void sunpos(double d, double *lon, double *r);
};

#endif

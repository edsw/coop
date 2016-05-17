//http://stjarnhimlen.se/comp/sunriset.c
#include <math.h>
#include "RTClib.h"

#ifndef Sol_Arduino
#define Sol_Arduino

class Sol
{
public:
    Sol();
    Sol(double lat, double lon, int gmtOffset, boolean dstTimeZone);
    void Update(DateTime dt);
    DateTime Sunrise();
    DateTime Sunset();
    
private:
    double _lat;
    double _lon;
    DateTime _dt;
    int _gmtOff;
    int _dstOff;
    boolean _dst;
    double _rise;
    double _set;
    double RADEG;
    double DEGRAD;
    double INV360;
    int days_since_2000_Jan_0(int y, int m, int d);
    double sind(double x);
    double cosd(double x);
    double tand(double x);
    double atand(double x);
    double asind(double x);
    double acosd(double x);
    double atan2d(double y, double x);
    void sunriset(int year, int month, int day, double lat, double lon, double altit, int upper_limb);
    double revolution(double x);
    double rev180(double x);
    double GMST0(double d);
    void sun_RA_dec(double d, double *RA, double *dec, double *r);
    void sunpos(double d, double *lon, double *r);
};

#endif
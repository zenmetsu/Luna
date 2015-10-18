/*
 * pebble.c
 * Sets up a Window object and pushes it onto the stack.
 */

#include <pebble.h>
#include <math.h>
#include "luna.h"

int initialized = 0;  
int updateCount = 0;
float updateDivision;

static Window *s_main_window;
static BitmapLayer *s_canvas_layer;
static TextLayer *s_text_layer;
static TextLayer *s_text2_layer;
static TextLayer *s_text3_layer;
static TextLayer *s_text4_layer;
static TextLayer *s_text5_layer;
static TextLayer *s_battery_layer;
struct tm *pt;
time_t then;

static GPath *s_luna_path;

double moonRange;


enum LocationKey {
  KEY_LONGITUDE = 0x0,         // TUPLE_FLOAT
  KEY_LATITUDE = 0x1,          // TUPLE_FLOAT
};

int32_t userLongitude,userLatitude = 0;

  
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

// The following arguments are multiples of the
// D  M  M' and F values.  
// D = Mean elongation of the moon.
// M = Mean anomaly of the sun.
// M'= Mean anomaly of the moon.
// F = Moon's argument of latitude.
//
// The following sixty terms all relate to periodic 
// factors that affect the longitude and altitude
// of the moon.  Some are quite small effects, such
// as influence from the moons of Jupiter.  Other are
// large.  I plan to update each term with its origin
// soon.  The coefficients in the second array are 
// values expressed in degrees multiplied by 1million.
//
// These terms are summed for latitude and altitude (or range)
// and are of the formulae:
//
// coeff[i] * sin(D*arg[i][0] + M*arg[i][1] + M'*arg[i][2] 
//                + F*arg[i][3]) = ∑longitude
// coeff[i] * cos(D*arg[i][0] + M*arg[i][1] + M'*arg[i][2] 
//                + F*arg[i][3]) = ∑range
// 
// Additionally, the coeffcient must be multiplied by the 
// value E^n where n is the absolute value of the M multiplier.
// E = 1 - 0.002516 * T - 0.0000074 * T * T
// This is because the mean anomaly of the sun is variable and 
// currently decreasing.

static const int argMult1 [60][4] = {
      { 0, 0, 1, 0},
      { 2, 0,-1, 0},
      { 2, 0, 0, 0},
      { 0, 0, 2, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 2},
      { 2, 0,-2, 0},
      { 2,-1,-1, 0},
      { 2, 0, 1, 0},
      { 2,-1, 0, 0},
      { 0, 1,-1, 0},
      { 1, 0, 0, 0},
      { 0, 1, 1, 0},
      { 2, 0, 0,-2},
      { 0, 0, 1, 0},
      { 0, 0, 1, 0},
      { 4, 0,-1, 0},
      { 0, 0, 3, 0},
      { 4, 0,-2, 0},
      { 2, 1,-1, 0},
      { 2, 1, 0, 0},
      { 1, 0,-1, 0},
      { 1, 1, 0, 0},
      { 2,-1, 1, 0},
      { 2, 0, 2, 0},
      { 4, 0, 0, 0},
      { 2, 0,-3, 0},
      { 0, 1,-2, 0},
      { 2, 0,-1, 2},
      { 2,-1,-2, 0},
      { 1, 0, 1, 0},
      { 2,-2, 0, 0},
      { 0, 1, 2, 0},
      { 0, 2, 0, 0},
      { 2,-2,-1, 0},
      { 2, 0, 1,-2},
      { 2, 0, 0, 2},
      { 4,-1,-1, 0},
      { 0, 0, 2, 2},
      { 3, 0,-1, 0},
      { 2, 1, 1, 0},
      { 4,-1,-2, 0},
      { 0, 2,-1, 0},
      { 2, 2,-1, 0},
      { 2, 1,-2, 0},
      { 2,-1, 0,-2},
      { 4, 0, 1, 0},
      { 0, 0, 4, 0},
      { 4,-1, 0, 0},
      { 1, 0,-2, 0},
      { 2,-1, 0,-2},
      { 0, 0, 2,-2},
      { 1, 1, 1, 0},
      { 3, 0,-2, 0},
      { 4, 0,-3, 0},
      { 2,-1, 2, 0},
      { 0, 2, 1, 0},
      { 1, 1,-1, 0},
      { 2, 0, 3, 0},
      { 2, 0,-1,-2}
}; 

static const signed long coefficientSin1 [60] =
 { 6288774, 1274027,  658314,  213618, -185116,
   -114332,   58793,   57066,   53322,   45758,
    -40923,  -34720,  -30383,   15327,  -12528,
     10980,   10675,   10034,    8548,   -7888,
     -6766,   -5163,    4987,    4036,    3994,
      3861,    3665,   -2689,   -2602,    2390,
     -2348,    2236,   -2120,   -2069,    2048,
     -1773,   -1595,    1215,   -1110,    -892,
      -810,     759,    -713,    -700,     691,
       596,     549,     537,     520,    -487,
      -399,    -381,     351,    -340,     330,
       327,    -323,     299,     294,       0  };

static const signed long coefficientCos1 [60] =
{-20905355,-3699111,-2955968, -569925,   48888,
     -3149,  246158, -152138, -170733, -204586,
   -129620,  108743,  104755,   10321,       0,
     79661,  -34782,  -23210,  -21636,   24208,
     30824,   -8379,  -16675,  -12831,  -10445,
    -11650,   14403,   -7003,       0,   10056,
      6322,   -9884,    5751,       0,   -4950,
      4131,       0,   -3958,       0,    3258,
      2616,   -1897,   -2117,    2354,       0,
         0,   -1423,   -1117,   -1571,   -1739,
         0,   -4421,       0,       0,       0,
         0,    1165,       0,       0,    8752};

static double sunMeanAnomaly(double T);
static void requestLocation(void);
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

// AppSync 


static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}


static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case KEY_LONGITUDE:
      userLongitude = -1 * new_tuple->value->int32;
      if (userLongitude < 0) {
        userLongitude = 360 + userLongitude;
      }
      break;
    case KEY_LATITUDE:
      userLatitude = new_tuple->value->int32;
      break;
  }
  initialized = 0;
  updateCount = 0;
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  //layer_mark_dirty(window_get_root_layer(s_main_window));
}
 

// Utility functions


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  //updateDivision = 60.0;
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
    layer_mark_dirty(window_get_root_layer(s_main_window));
}


static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  static char s_battery_buffer[32];
  if (new_state.is_charging) {
    text_layer_set_text_color(s_battery_layer, GColorCeleste);
  } else {
    text_layer_set_text_color(s_battery_layer, GColorVividCerulean);
  }
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}


static void focus_handler(bool in_focus) {
  if (in_focus) {
  }
  else {  
    initialized = 0;
    updateCount = 0;
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  }
}


// No floating point support in text_layer_set_text()?!?  Fuck it...
void ftoa(char* str, double val, int precision) {
  int currentDigit = 0;
  //  start with positive/negative
  if (val < 0) {
    *(str++) = '-';
    val = -val;
  }
  //  integer value
  snprintf(str, 12, "%d", (int) val);
  str += strlen(str);
  val -= (int) val;
  //  decimals
  if ((precision > 0) && (val >= .00001)) {
    //  add period
    *(str++) = '.';
    //  loop through precision
    for (int i = 0;  i < precision;  i++)
      if (val > 0) {
        val *= 10;
        currentDigit = (int) (val + ((i == precision - 1) ? .5 : 0));
        *(str++) = '0' + ((currentDigit > 9) ? 9 : currentDigit);
        val -= (int) val;
      } else
        break;
  }
  //  terminate
  *str = '\0';
}


float sqrtx(const float num) {
  const uint MAX_STEPS = 40;
  const float MAX_ERROR = 0.001;
  
  float answer = num;
  float ans_sqr = answer * answer;
  uint step = 0;
  while((ans_sqr - num > MAX_ERROR) && (step++ < MAX_STEPS)) {
    answer = (answer + (num / answer)) / 2;
    ans_sqr = answer * answer;
  }
  return answer;
}


void moonTime(char* str, double val) {
  val = val * 24.0 / 360.0;
  int hours = (int)val;

  int minutes = (int)(60.0 * (val - hours));
  

  hours = hours + 12;
  if (hours > 23) {
    hours = hours - 24;
  }  
  if (minutes < 10) {
    snprintf(str, 24, "%02d:0%d", hours, minutes);
  } else {
    snprintf(str, 24, "%02d:%2d", hours, minutes);
  }
  
}


static double degrees(double d) {
  return (d * 360.0 / (2.0 * M_PI));
}


static double radians(double d) {
  int multiple = (int)(d / 360.0);
  if (d < 0) {
    return (d + 360.0 - (360.0 * multiple)) * 2.0 * M_PI / 360.0;
  } else {
    return (d - (360.0 *multiple)) * 2.0 * M_PI / 360.0;
  }
}


static double normDegrees(double d) {
  int multiple = (int)(d / 360.0);
  if (d < 0){
    return d + 360.0 - (360.0 * multiple);
  } else {
    return d - (360.0 * multiple);
  }
}


static double normRadians(double d) {
  int multiple = (int)(d / (2.0 * M_PI));
  if (d < 0){
    return d + (2.0 * M_PI) - ((2.0 * M_PI) * multiple);
  } else {
    return d - ((2.0 * M_PI) * multiple);
  }
}


static double powa(double base, int power){
  double x=1;
  for(int i = 1;i<=power;i++){
    x *= base;
  }
  return x;
}


static long facta(int n){
  double x=1;
  for(int i = 1;i<=n;i++){
    x *= i;
  }
  return x;
}


double fabs(double d) {
  if (d > -d){
    return d;
  } else {
    return -d;
  }
}


static double sinx(double d) {
  d = normDegrees(d);
  int mult = 1;
  if (d > M_PI){
    d = d - M_PI;
    mult = -1;
  }
  int iteration;
  double x = 0.0;
  double y = 0.0;
  for (iteration=0;iteration < 6;iteration++) {
    x = (double)(powa(-1.0,iteration) * powa(d, 2*iteration + 1) / (double)facta(2*iteration + 1));
    y += x;
    if (fabs(x) < 0.0000001) {
      break;
    }
  }  
  return (double)mult * y;
}


static double cosx(double d) {
  d = normDegrees(d);
  int mult = 1;
  if (d > M_PI) {
    d = d - M_PI;
    mult = -1;
  }
  int iteration;
  double x = 0.0;
  double y = 0.0;
  for (iteration=0;iteration < 6;iteration++) {
    x = (double)(powa(-1.0,iteration) * powa(d, 2*iteration) / (double)facta(2*iteration));
    y += x;
    if (fabs(x) < 0.000001) {
      break;
    }
  }  
  return (double)mult * y;
}


// Calculate Julian Date from a time struct
// Meeus - Astronomical Algorithms - formula 7.1
static double DateToJD(struct tm *t) {
  int M = t->tm_mon + 1 > 2 ? t->tm_mon + 1 : t->tm_mon + 13;
  int Y = t->tm_mon + 1 > 2 ? t->tm_year + 1900 : t->tm_year + 1899;
  double D = t->tm_mday + t->tm_hour/24.0 + t->tm_min/1440.0 + t->tm_sec/86400.0;
  int B = 2 - (int)Y/100 + (int)Y/400;

  return (int) (365.25*(Y + 4716)) + (int) (30.6001*(M + 1)) + D + B - 1524.5;
}


// Calculate time T, measured in Julian centuries from the 
// epoch J2000.0 (JDE 2451545.0)   
// Meeus - Astronomical Algorithms - formula 22.1
static double JDtoT(double *JD) {
  return (double)(*JD - 2451545.0)/36525.0;
}


// Calculate the sun's mean longitude, measured in degrees [Lo]
// Meeus - Astronomical Algorithms - formula 25.2
static double sunMeanLongitude(double T) {
  return 280.46646 
         + 36000.76983 * T 
         + 0.0003032 * T * T;
}


// Calculate the sun's equation of center, measured in degrees [L']
// Meeus - Astronomical Algorithms - formula 47.1
static double sunEquationCenter(double T) {
  return degrees((1.914602 - 0.004817 * T - 0.000014 * T * T)
         * sinx(radians(sunMeanAnomaly(T)))
         + (0.019993 - 0.000101 * T)
         * sinx(radians(2.0 * sunMeanAnomaly(T)))
         + 0.000289 * sinx(radians(3.0 * sunMeanAnomaly(T))));
}


// Calculate the moon's mean longitude, measured in degrees [L']
// Meeus - Astronomical Algorithms - formula 47.1
static double moonMeanLongitude(double T) {
  return 218.3164477 
         + 481267.88123421 * T 
         - 0.0015786 * T * T 
         + T * T * T/538841.0 
         - T * T * T * T/65194000.0;
}


// Calculate the moon's mean elongation, measured in degrees [D]
// Meeus - Astronomical Algorithms - formula 47.2
static double moonMeanElongation(double T) {
  return 297.8501921 
         + 445267.1114034 * T 
         - 0.0018819 * T * T 
         + T * T * T/544868.0 
         - T * T * T * T/113065000.0;
}


// Calculate the sun's mean anomaly, measured in degrees [M]
// Meeus - Astronomical Algorithms - formula 47.3
static double sunMeanAnomaly(double T) {
  return 357.5291092
         + 35999.0502909 * T 
         - 0.0001536 * T * T 
         + T * T * T / 24490000.0;
}


// Calculate the moon's mean anomaly, measured in degrees [M']
// Meeus - Astronomical Algorithms - formula 47.4
static double moonMeanAnomaly(double T) {
  return 134.9633964
         + 477198.8675055 * T 
         + 0.0087414 * T * T 
         + T * T * T / 69699.0
         - T * T * T * T / 14712000.0;
}


// Calculate the moon's argument of latitude, measured in degrees [F]
// Meeus - Astronomical Algorithms - formula 47.5
static double moonArgLatitude(double T) {
  return 93.2720950
         + 483202.0175233 * T 
         - 0.0036539 * T * T 
         + T * T * T/3526000.0
         + T * T * T * T/863310000.0;
}


static double Eccentricity(double T) {
  return 1.0 - 0.002516 * T - 0.0000074 * T * T;
}


static double sigmaMoonLongitude(double T) {
  //T = -0.077221081451; //debug value
  double sigmaLongitude = 0.0;
  double A1 = radians(119.75 + 131.849 * T);
  double A2 = radians(53.09 +479264.290 * T);
  
  double E  = Eccentricity(T);
  double L  = moonMeanLongitude(T);
  double D  = moonMeanElongation(T);
  double M  = sunMeanAnomaly(T);
  double Mm = moonMeanAnomaly(T);
  double F  = moonArgLatitude(T);
  
  for(int term=0;term < 60;term++){
    sigmaLongitude += coefficientSin1[term] * powa(E,(int)fabs(argMult1[term][1])) *
                       sinx(radians((double)argMult1[term][0] * D +
                                    (double)argMult1[term][1] * M +
                                    (double)argMult1[term][2] * Mm +
                                    (double)argMult1[term][3] * F));
  }

  sigmaLongitude += radians(3958.0 * sinx(A1));
  sigmaLongitude += radians(1962.0 * sinx(radians(L - F)));
  sigmaLongitude += radians( 318.0 * sinx(A2));

  return normDegrees(L + sigmaLongitude / 1000000.0); 
}


static double sigmaMoonRange(double T) {
  //T = -0.077221081451; //debug value
  double sigmaRange = 0.0;
  
  double E  = Eccentricity(T);

  double D  = moonMeanElongation(T);
  double M  = sunMeanAnomaly(T);
  double Mm = moonMeanAnomaly(T);
  double F  = moonArgLatitude(T);
  
  for(int term=0;term < 60;term++){
    sigmaRange += coefficientCos1[term] * powa(E,(int)fabs(argMult1[term][1])) *
                       cosx(radians((double)argMult1[term][0] * D +
                                    (double)argMult1[term][1] * M +
                                    (double)argMult1[term][2] * Mm +
                                    (double)argMult1[term][3] * F));
  }
  
  return 0.62137119 * (385000.56 + (sigmaRange / 1000.0)); 
}


static double moonRA(double L){
  int y,x;
  float g;
  y = (int16_t)(10000 * sinx(radians(L)) * cosx(radians(obliquityE)));
  x = (int16_t)(10000 * cosx(radians(L)));
  g = atan2_lookup(y,x);
  g = 360.0 * g / TRIG_MAX_ANGLE;
  return g;
}

static double moonOrbitalSpeed(double Range){
  Range = Range * 1609.344;  // convert miles to meters
  float moonMu = 398600000000000.8000;
  double moonSemiMajor = 384400000.0;
  return 3600.0 * (sqrtx(moonMu * ((2.0 / Range) - (1.0 / moonSemiMajor)))) / 1609.344;
}


static double sunRA(double T){ 
  //T = -0.024012092;
  float x,y;
  float d = T * 36525.0;
  float L = 280.461 + 0.9856474 * d;
  float g = sunMeanAnomaly(T);
  float lambda = L + 1.915 * sinx(radians(g)) + 0.020 * sinx(radians(2*g));
  y = cosx(radians(obliquityE)) * sinx(radians(lambda));
  x = cosx(radians(lambda));

  g = atan2_lookup((int16_t)(10000 * y),(int16_t)(10000 * x));
  g = 360.0 * g / TRIG_MAX_ANGLE;
  
  return g;
}


static double greenwichSiderealTime(double T, struct tm *t) {
  double offset = (double)(t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec); 
  
  offset = 360.0 * offset / 86400.0;
  return normDegrees(100.46061837 + (36000.770053608 * T) 
         + (0.000387933 * T * T) - (T * T * T / 38710000.0)
         + 1.00273790935 * offset);
}


static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = gmtime(&now);
  struct tm *lt = localtime(&now);
    
  double JD = DateToJD(t);
  double T = JDtoT(&JD);
  
  //T = -0.077221081451;
  
  static char buf[24] = "";
  static char buf2[32] = "";
  static char buf3[24] = "";
  static char buf4[24] = "";
  static char buf5[12] = "";
  static char buf6[16] = "";
  
  int moonOrbitRadius = 56;
  int hashLength = 3;
  
  double moonLongitude;
  double moonRightAscension;
  double moonHourAngle;
  double moonAltitude;
  double moonDoppler = 0;
  double moonSpeed;
  
  double sunRightAscension;
  double sunHourAngle;
  
  double elapsedTime = difftime(now,then);
  
  float moonX,moonY;
  int pointX,pointY;
  
  
  if (elapsedTime < 1.0) {
    elapsedTime = 1.0;
  }
  
  

  
  
  
  GRect bounds = layer_get_bounds(this_layer);
  
  // Get the center of the screen (non full-screen)
  GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));
  
  // Draw the moon's orbit
  graphics_context_set_stroke_width(ctx,2);
  graphics_context_set_stroke_color(ctx, GColorTiffanyBlue);
  graphics_draw_circle(ctx, center, moonOrbitRadius);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx,1);
  graphics_draw_line(ctx, GPoint((bounds.size.w/2) - hashLength - moonOrbitRadius ,(bounds.size.h/2)), 
                          GPoint((bounds.size.w/2) + hashLength - moonOrbitRadius ,(bounds.size.h/2)));
  graphics_draw_line(ctx, GPoint((bounds.size.w/2) - hashLength + moonOrbitRadius ,(bounds.size.h/2)), 
                          GPoint((bounds.size.w/2) + hashLength + moonOrbitRadius ,(bounds.size.h/2)));
  graphics_draw_line(ctx, GPoint((bounds.size.w/2) ,(bounds.size.h/2) - hashLength - moonOrbitRadius), 
                          GPoint((bounds.size.w/2) ,(bounds.size.h/2) + hashLength - moonOrbitRadius));
  graphics_draw_line(ctx, GPoint((bounds.size.w/2) ,(bounds.size.h/2) - hashLength + moonOrbitRadius), 
                          GPoint((bounds.size.w/2) ,(bounds.size.h/2) + hashLength + moonOrbitRadius)); 
  
  // Draw the moon
  moonLongitude = sigmaMoonLongitude(T);
  moonAltitude = sigmaMoonRange(T);
  moonDoppler = 3600.0 * (moonAltitude - moonRange) / elapsedTime; //3600
  moonRange = moonAltitude;
  moonSpeed = moonOrbitalSpeed(moonAltitude);
  
  moonRightAscension = moonRA(moonLongitude);
  moonHourAngle = normDegrees(greenwichSiderealTime(T,t) - (float)userLongitude - moonRightAscension);

  moonX = (float)sinx(radians(moonHourAngle));
  moonY = (float)cosx(radians(moonHourAngle));
  pointX = (int)(1.0 * moonX * moonOrbitRadius);
  pointY = (int)(-1.0 * moonY * moonOrbitRadius);


  sunRightAscension = sunRA(T);
  sunHourAngle = normDegrees(greenwichSiderealTime(T,t) - (float)userLongitude - sunRightAscension);

  // Draw Velocity hints
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_draw_line(ctx, GPoint(pointX + (bounds.size.w/2)
                                ,pointY + (bounds.size.h/2)),
                          GPoint((int)(pointX + (bounds.size.w/2) + moonX * moonDoppler / 5.0),
                                 (int)(pointY + (bounds.size.h/2) + moonY * moonDoppler / -5.0)));
  
  // dark side
  graphics_context_set_antialiased(ctx,0);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx,GPoint(pointX + (bounds.size.w/2)
                             ,pointY + (bounds.size.h/2)),5);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_fill_color(ctx, GColorDarkGray);

  gpath_rotate_to(s_luna_path, TRIG_MAX_ANGLE * ((sunHourAngle / 360.0) + 0.25));
  gpath_move_to(s_luna_path, GPoint((bounds.size.w/2) + pointX,(bounds.size.h/2) + pointY));
    
  gpath_draw_filled(ctx, s_luna_path);
  
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_fill_color(ctx, GColorWhite);

  gpath_rotate_to(s_luna_path, TRIG_MAX_ANGLE * ((sunHourAngle / 360.0) - 0.25));
  gpath_draw_filled(ctx, s_luna_path);
  graphics_context_set_antialiased(ctx,1);
  
  
  // Display Moon Time
  
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
  
  text_layer_set_text_color(s_text_layer, GColorWhite);
  
 
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter); 
  moonTime(buf,moonHourAngle);
  text_layer_set_text(s_text_layer, buf);   
  
  text_layer_set_text_alignment(s_text2_layer, GTextAlignmentRight); 
  text_layer_set_font(s_text2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  
  snprintf(buf2, 12, "%d mi\n", (int)moonRange);
  ftoa(buf3,moonDoppler,1);
  strcat(buf2, buf3);
  strcat(buf2, " mph");
  text_layer_set_text(s_text2_layer, buf2); 
  
  text_layer_set_text_color(s_text3_layer, GColorBrightGreen);
  snprintf(buf6,18, "Lon:%ld Lat:%ld", userLongitude, userLatitude);
  text_layer_set_text(s_text3_layer, buf6);
  
  text_layer_set_font(s_text4_layer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_color(s_text4_layer, GColorIcterine);
  snprintf(buf4,12, "%02d:%02d", lt->tm_hour, lt->tm_min);
  text_layer_set_text(s_text4_layer, buf4);
  
  text_layer_set_text_alignment(s_text5_layer, GTextAlignmentLeft); 
  text_layer_set_font(s_text5_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_text5_layer, GColorTiffanyBlue);
  ftoa(buf5,moonSpeed,1);

  strcat(buf5, " mph");
  text_layer_set_text(s_text5_layer, buf5);  
  
  
  if (initialized < 1) {
    updateCount += 1;
    if (updateCount > 5) {
      initialized += 1;
      tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    }
  } 
  pt = gmtime(&now);
  then = time(NULL);
  
  /*
  if ((lt->tm_min == 0 || lt->tm_min == 55) && lt->tm_sec == 0){
    initialized = 0;
    updateCount = 0;
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
    requestLocation();
  }
  */
 
}


static void main_window_load(Window *window) {
  // Create Window's child Layers here
  
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  // Create Layer
  s_canvas_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_canvas_layer));

  // Set the update_proc
  layer_set_update_proc(bitmap_layer_get_layer(s_canvas_layer), canvas_update_proc);
  
  // Create First Text Layer - Middle, used for time
  s_text_layer = text_layer_create(GRect(0, 63, window_bounds.size.w, 90));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_text_layer, "No time yet.");
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_text_layer, GColorClear);
  
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  
// Create Second Text Layer - Top, used for orbital elements
  s_text2_layer = text_layer_create(GRect(3, -4, window_bounds.size.w - 6, 36));
  text_layer_set_font(s_text2_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_text2_layer, "No data yet.");
  text_layer_set_overflow_mode(s_text2_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_text2_layer, GColorClear);
  text_layer_set_text_color(s_text2_layer, GColorChromeYellow);
  
  layer_add_child(window_layer, text_layer_get_layer(s_text2_layer)); 
  
// Create Third Text Layer - Bottom, used for location
  s_text3_layer = text_layer_create(GRect(0, window_bounds.size.h -20, window_bounds.size.w, 20));
  text_layer_set_font(s_text3_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_text3_layer, "No data yet.");
  text_layer_set_overflow_mode(s_text3_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_text3_layer, GColorClear);
  text_layer_set_text_alignment(s_text3_layer, GTextAlignmentCenter); 
  
  layer_add_child(window_layer, text_layer_get_layer(s_text3_layer));   

// Create Fourth Text Layer - Middle, used for standard time
  s_text4_layer = text_layer_create(GRect(0, 96, window_bounds.size.w, 20));
  text_layer_set_font(s_text4_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_text4_layer, "No data yet.");
  text_layer_set_overflow_mode(s_text4_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_text4_layer, GColorClear);
  text_layer_set_text_alignment(s_text4_layer, GTextAlignmentCenter); 
  
  layer_add_child(window_layer, text_layer_get_layer(s_text4_layer));   
  
// Create Fifth Text Layer - Upper Left, used for orbital speed
  s_text5_layer = text_layer_create(GRect(3, -4, window_bounds.size.w - 6, 36));
  text_layer_set_font(s_text5_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_text5_layer, "No data yet.");
  text_layer_set_overflow_mode(s_text5_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_text5_layer, GColorClear);
  text_layer_set_text_alignment(s_text5_layer, GTextAlignmentCenter); 
  
  layer_add_child(window_layer, text_layer_get_layer(s_text5_layer));  
  
  
// Create Battery Text Layer 
  s_battery_layer = text_layer_create(GRect(window_bounds.size.w -30, 152, 30, 20));
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_battery_layer, "No data yet.");
  text_layer_set_overflow_mode(s_battery_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight); 
  text_layer_set_text_color(s_battery_layer, GColorVividCerulean);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer)); 
  
  battery_handler(battery_state_service_peek());
  
  app_focus_service_subscribe(focus_handler);
}


static void requestLocation(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  //dict_write_int(iter, 1, &value, sizeof(int), true);
  //dict_write_end(iter);
  dict_write_uint8(iter, 0, 0);
  

  app_message_outbox_send();
}


static void main_window_unload(Window *window) {
  // Destroy Window's child Layers here
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_text2_layer);
  text_layer_destroy(s_text3_layer);
  text_layer_destroy(s_text4_layer);
  text_layer_destroy(s_text5_layer);
  text_layer_destroy(s_battery_layer);
  bitmap_layer_destroy(s_canvas_layer);
}






static void init() {
  time_t now = time(NULL);
  
  // Create main Window  
  updateCount = 0;
  updateDivision = 1.0;
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  s_luna_path = gpath_create(&LUNA_PATH_POINTS);
  //tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  
  battery_state_service_subscribe(battery_handler);
  
  Tuplet initial_values[] = {
    TupletInteger(KEY_LONGITUDE, (int32_t) 0),
    TupletInteger(KEY_LATITUDE,  (int32_t) 0),
  };   

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), 
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL
  );  
  
  requestLocation();
  pt = gmtime(&now);
  then = time(NULL);
}


static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
  //app_sync_deinit(&s_sync);
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
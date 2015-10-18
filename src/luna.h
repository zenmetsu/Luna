#pragma once
#include <pebble.h>
  
/*  
static const GPathInfo LUNA_PATH_POINTS = {
  26, (GPoint []){
    {0,0},  {0,8},  {1,8},  {2,8},  {3,7},
    {4,7},  {5,6},  {6,6},  {6,5},  {7,4},
    {7,3},  {8,2},  {8,1},  {8,0},  {8,-1},
    {8,-2}, {7,-3}, {7,-4}, {6,-5}, {6,-6},
    {5,-6}, {4,-7}, {3,-7}, {2,-8}, {1,-8},
    {0,-8}
  }
};
*/


static const GPathInfo LUNA_PATH_POINTS = {
  30, (GPoint []){
    {0,0},   {0,10},  {1,10},  {2,10},  {3,10},
    {4,9},    {5,9},   {6,8},   {7,7},   {8,6},
    {9,5},    {9,4},  {10,3},  {10,2},  {10,1},
    {10,0}, {10,-1}, {10,-2}, {10,-3},  {9,-4},
    {9,-5},  {8,-6},  {7,-7},  {6,-8},  {5,-9},
    {4,-9}, {3,-10}, {2,-10}, {1,-10},  {0,-10}
  }
};  
  

#ifndef M_PI 
  #define M_PI 3.1415926535897932384626433832795 
#endif

// adjustment arguments in degrees
// a1 = venus
// a2 = jupiter
static const double a1a = 119.75;
static const double a1b = 131.849;
static const double a2a = 53.09;
static const double a2b = 479264.290;
static const double a3a = 313.45;
static const double a3b = 481266.484;

static const double obliquityE = 23.4392911;  

#include <math.h>
#include "Coordinates.h"

float getAngle(coordinate point_of_interest, coordinate reference) {
  float angle = atan2(point_of_interest.y - reference.y,
                      point_of_interest.x - reference.x) * 180 / M_PI;
  if (angle < 0) angle += 360;
  return angle;
}

float getDeltaAngle(float angle_of_interest, float reference_angle) {
  float delta_angle = reference_angle - angle_of_interest;
  if (fabs(delta_angle) > 180) {
    delta_angle += (delta_angle < 0) ? 360 : -360;
  }
  return delta_angle;
}

float getDistance(coordinate a, coordinate b) {
  return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}
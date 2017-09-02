#ifndef COORDINATES_H
#define COORDINATES_H

struct coordinate {
  float x, y, z;
};

extern coordinate target_coord, old_coord, new_coord;
extern float car_bearing, target_bearing,
             delta_angle, delta_distance;
             
// Returns the angle of coordinate point_of_interest
// relative to coordinate reference in degrees.
float getAngle(coordinate point_of_interest, coordinate reference);

// Returns the angle difference between an angle of interest and a
// reference angle. Negative values mean that one should turn left,
// while positive values mean one should turn right.
float getDeltaAngle(float angle_of_interest, float reference_angle);
float getDistance(coordinate a, coordinate b);
#endif
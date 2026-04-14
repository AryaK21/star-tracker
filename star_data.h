#ifndef STAR_DATA_H
#define STAR_DATA_H

typedef struct {
    int id;
    double ra;      // Right Ascension (Absolute)
    double dec;     // Declination (Absolute)
    float magnitude;
    double alt;     // Altitude (Relative to user)
    double az;      // Azimuth (Relative to user)
} Star;

#endif

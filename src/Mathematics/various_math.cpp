//
//  various_math.cpp
//  Trusses
//
//  Created by Patrick on 24/03/2015.
//  Copyright (c) 2015 Patrick Szmucer. All rights reserved.
//

#include <stdio.h>
#include <cstdlib>
#include "various_math.h"

double random(double range)
{
    double r = 1.0 * rand() / RAND_MAX;
    return r * 2 * range - range;
}

double abs_d(double x)
{
    if (x >= 0.0)
        return x;
    else
        return -x;
}

double round(double x)
{
    x += 0.5;
    return floor(x);
}

double min(double a, double b)
{
    if (a >= b)
        return b;
    else
        return a;
}

double max(double a, double b)
{
    if (a >= b)
        return a;
    else
        return b;
}
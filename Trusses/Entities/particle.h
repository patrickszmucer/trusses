//
//  particle.h
//  Trusses
//
//  Created by Patrick Szmucer on 30/12/2014.
//  Copyright (c) 2014 Patrick Szmucer. All rights reserved.
//

#ifndef __Trusses__particle__
#define __Trusses__particle__

#include <vector>
#include "math.h"
#include "slot_map.h"
#include "fixed_size.h"

struct Particle
{
    int id_;
    Vector2d position_;
    Vector2d prev_position_;
    Vector2d prev_position_verlet_;
    Vector2d velocity_;
    Vector2d acceleration_;
    Vector2d external_acceleration_; // Added by dragging the particle
    double mass_;
    bool fixed_;
    bool dragged_;
    std::vector<int> bars_connected;
    
    bool trace_on;
    FixedSizeContainer<Vector2d> trace;
    
    void update();
    void impose_boundaries();
    
    static int create(double a, double b, bool fixed);
    static int destroy(int removed_id);
    
private:
    Particle(double a, double b): trace(500) {position_.x = a; position_.y = b;}
};

void print_particles();
void reset_particles();

extern SlotMap<Particle> particles;

#endif /* defined(__Trusses__particle__) */
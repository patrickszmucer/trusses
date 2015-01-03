//
//  particle.cpp
//  Oscillator
//
//  Created by Patrick Szmucer on 30/12/2014.
//  Copyright (c) 2014 Patrick Szmucer. All rights reserved.
//

#include "particle.h"
#include "physics.h"

std::vector<Particle> particles;
int particles_number = 0;

Particle Particle::create(double a, double b, bool fixed)
{
    // TODO
    // Check if a particle of this id already exists
    
    Particle p(a, b);
    
    p.prev_position_ = p.position_ - delta_t * p.velocity_;
    p.prev_position_verlet_ = p.position_ - delta_t * p.velocity_;
    p.velocity_ = Vector2d(0.0, 0.0);
    p.acceleration_ = Vector2d(0.0, 0.0);
    p.external_acceleration_ = Vector2d(0.0, 0.0);
    p.mass_ = 1.0;
    
    p.fixed_ = (fixed) ? true : false;
    p.dragged_ = false;
    
    p.id_ = particles_number;
    particles_number++;
    
    return p;
}

Vector2d position_;
Vector2d prev_position_;
Vector2d prev_position_verlet_;
Vector2d velocity_;
Vector2d acceleration_;
Vector2d external_acceleration_; // Added by dragging the particle
double mass_;
int id_;
bool fixed_;
bool dragged_;
//
//  particle.cpp
//  Trusses
//
//  Created by Patrick Szmucer on 30/12/2014.
//  Copyright (c) 2014 Patrick Szmucer. All rights reserved.
//

#include "particle.h"
#include "physics.h"
#include "bar.h"
#include "wall.h"
#include "temporary_label.h"

PSlotMap<Particle*> particles;

void Particle::update()
{
    if (!fixed_)
    {
        // External acceleration is the acceleration added by dragging with a mouse
        acceleration_ = external_acceleration_;
        
        // Remember the presious position
        prev_position_ = position_;
        
        // Trace the previous position
        if (trace_on)
            trace.add(position_);
        
        if (gravity)
            acceleration_ += Vector2d(0.0, -GRAVITY);
        
        // Verlet integration
        Vector2d next_position = 2 * position_ - prev_position_verlet_ + pow(delta_t, 2) * acceleration_;
        velocity_ = (0.5 / delta_t) * (next_position - prev_position_);
        prev_position_verlet_ = position_;
        position_ = next_position;
    }
}

void print_particles()
{
    for (int i = 0; i < particles.size(); i++)
    {
        Particle* p = particles.at(i);
        std::cout << "Particle " << p->id_ << ": ";
        for (int j = 0; j < p->bars_connected.size(); j++)
        {
            std::cout << p->bars_connected[j] << " ";
        }
        std::cout << std::endl;
    }
}

void reset_particles()
{
    particles.clear();
}

int Particle::destroy(int obj_id)
{
    if (!particles.exists(obj_id))
    {
        issue_label("This particle does not exist", WARNING_LABEL_TIME);
        return 1;
    }
    
    // Remove all the bars that this particle was connected to
    Particle* this_p = particles[obj_id];
    size_t no_bars_connected = this_p->bars_connected.size();
    for (int i = 0; i < no_bars_connected; i++)
        Bar::destroy(this_p->bars_connected.back());
    
    int result = particles.remove(obj_id);
    return result;
}
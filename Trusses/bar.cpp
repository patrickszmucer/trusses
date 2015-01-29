//
//  bar.cpp
//  Oscillator
//
//  Created by Patrick Szmucer on 03/01/2015.
//  Copyright (c) 2015 Patrick Szmucer. All rights reserved.
//

#include "bar.h"
#include "particle.h"
#include "physics.h"
#include "graphics.h"

SlotMap<Bar> bars;

int Bar::create(int id1, int id2, double e, double temp)
{
    if (id1 == id2)
    {
        std::cout << "Canot create a bar connecting the same particle" << std::endl;
        return -1;
    }
    
    if (!particles.exists(id1) || !particles.exists(id2))
    {
        std::cout << "One or more required particles don't exist" << std::endl;
        return -1;
    }
    
    // TODO
    // Write a method that searches contents of a vector (or use an existing one)
    int no_bars = (int)particles[id1].bars_connected.size();
    for (int i = 0; i < no_bars; i++)
    {
        int bar_id = particles[id1].bars_connected[i];
        if ( bars[bar_id].p1_id == id2 || bars[bar_id].p2_id == id2)
        {
            std::cout << "Bar between these particles already exists" << std::endl;
            return -1;
        }
    }
    
    Bar new_bar(id1, id2);
    
    // This sets temperature, r0 and stiffness
    new_bar.temperature = temp; // Load every bar at room temperature (for now)
    new_bar.set_strain(e);
    new_bar.stiffness = -(1.0 - STIFFNESS_AT_TM) * new_bar.temperature / MELTING_POINT + 1.0; // Stiffness is 1 at 0K
    new_bar.color = vec3(1.0, 1.0, 1.0);
    
    int new_id = bars.add(new_bar);
    
    // Particles have to know which bars are connected to them
    particles[id1].bars_connected.push_back(new_id);
    particles[id2].bars_connected.push_back(new_id);

    return new_id;
}

void Bar::set_temperature(double t)
{
    temperature = t;
    r0 = r_0K * (1 + THERMAL_COEFF * temperature); // Assume linear expansion with temperature
    stiffness = -(1.0 - STIFFNESS_AT_TM) * temperature / MELTING_POINT + 1.0; // Stiffness is 1 at 0K
    
    // Set the appropriate color based on temperature
    //color = hsv_to_rgb(vec3(temperature/MELTING_POINT * 360, 1.0, 1.0)); // TODO
    
    double temp_chunk = MELTING_POINT / 3.0;
    if (temperature <= temp_chunk)
        color = vec3(temperature/temp_chunk, 0, 0);
    else if (temperature <= 2 * temp_chunk)
        color = vec3(1.0, (temperature - temp_chunk)/temp_chunk, 0.0);
    else if (temperature <= 3 * temp_chunk)
        color = vec3(1.0, 1.0, (temperature - 2 * temp_chunk)/temp_chunk);
    else
        color = vec3(1.0, 1.0, 1.0);
}

double Bar::get_temperature() const
{
    return temperature;
}

void Bar::set_strain(double e)
{
    r0 = length() / (e + 1.0);
    r_0K = r0 / (1 + THERMAL_COEFF * temperature);
}

double Bar::get_strain() const
{
    return extension() / r0;
}

int Bar::destroy(int obj_id)
{
    if (!bars.exists(obj_id))
    {
        std::cout << "This bar doesn't exist" << std::endl;
        return 1;
    }
    
    Particle* p1 = &particles[bars[obj_id].p1_id];
    Particle* p2 = &particles[bars[obj_id].p2_id];
    
    for (int i = 0; i < p1->bars_connected.size(); i++)
    {
        if (p1->bars_connected[i] == obj_id)
        {
            p1->bars_connected[i] = p1->bars_connected.back();
            p1->bars_connected.pop_back();
        }
    }
    for (int i = 0; i < p2->bars_connected.size(); i++)
    {
        if (p2->bars_connected[i] == obj_id)
        {
            p2->bars_connected[i] = p2->bars_connected.back();
            p2->bars_connected.pop_back();
        }
    }
    
    bars.remove(obj_id);
    
    return 0;
}

double Bar::length() const
{
    Vector2d pos1 = particles[p1_id].position_;
    Vector2d pos2 =particles[p2_id].position_;
    double ans = (pos1 - pos2).abs();
    return ans;
}

Vector2d Bar::unit12() const
{
    return (particles[p1_id].position_ - particles[p2_id].position_).norm();
}

Vector2d Bar::unit21() const
{
    return -unit12();
}

double Bar::extension() const
{
    return length() - r0;
}

void Bar::impose_constraint()
{
    int particle_location(int id);
    Particle* p1 = &particles[p1_id];
    Particle* p2 = &particles[p2_id];
    
    double ext = extension();
    
    double im1 = 1/p1->mass_;
    double im2 = 1/p1->mass_;
    float mult1 = (im1 / (im1 + im2)) * stiffness;
    float mult2 = stiffness - mult1;
    
    // TODO: A connection between fixed and not fixed particle is probably not handled correctly.
    // If one particle is fixed, the other should move two times further in the direction of the fixed particle.
    if (!p1->fixed_)
        p1->position_ += mult1 * ext * unit21();
    if (!p2->fixed_)
        p2->position_ += mult2 * ext * unit12();
}

// Returns 1 if bar will be destroyed, 0 otherwise
int Bar::update()
{
    // Temperature expansion
    if (abs_d(temperature - environment_temp) > SMALL_NUM)
        set_temperature( temperature + delta_t/10.0 * (environment_temp - temperature) );
    
    // Destroy bars which are extended by too much
    double ext = extension() / r0;
    if (abs_d(ext) > MAX_STRAIN || (temperature >= MELTING_POINT && random(1.0) > (1 - (temperature - MELTING_POINT) / 10000.0)))
        return 1;
    return 0;
}

void Bar::split(unsigned int n_parts)
{
    if (n_parts < 2)
    {
        std::cout << "Cannot divide bar in less than 2 parts" << std::endl;
        return;
    }
    
    int id_start = p1_id;
    int id_end = p2_id;
    Vector2d pos_start = particles[id_start].position_;
    Vector2d pos_end = particles[id_end].position_;
    
    double temp = temperature;
    double new_r0 = r0 / n_parts;
    double new_r_0K = r_0K / n_parts;
    Vector2d Dr = (pos_end - pos_start) / n_parts;
    
    // Create new particles
    std::vector<int> new_ids;
    for (int i = 1; i < n_parts; i++)
    {
         new_ids.push_back( Particle::create(pos_start.x + i * Dr.x, pos_start.y + i * Dr.y, false) );
    }
    
    // Connect particles with bars
    // Don't delete the first one, but instead modify it
    for (int i = 0; i < new_ids.size() + 1; i++)
    {
        int new_bar_id;
        
        if (i == 0)
        {
            p2_id = new_ids[0];
            new_bar_id = id_;
        }
        else if (i == new_ids.size())
            new_bar_id = Bar::create(new_ids.back(), id_end, 0.0, ROOM_TEMPERATURE);
        else
            new_bar_id = Bar::create(new_ids[i-1], new_ids[i], 0.0, ROOM_TEMPERATURE);
        
        bars[new_bar_id].r0 = new_r0;
        bars[new_bar_id].r_0K = new_r_0K;
        bars[new_bar_id].temperature = temp;
    }
    
}

void reset_bars()
{
    bars.clear();
}

void print_bars()
{
    SlotMap<Bar>::iterator bars_it;
    for (bars_it = bars.begin(); bars_it != bars.end(); bars_it++)
    {
        std::cout << "Bar " << bars_it->id_ << std::endl;
    }
}
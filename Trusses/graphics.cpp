//
//  graphics.cpp
//  Trusses
//
//  Created by Patrick Szmucer on 30/12/2014.
//  Copyright (c) 2014 Patrick Szmucer. All rights reserved.
//

#include "graphics.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/glut.h>
#endif
#include <sstream>

#include "particle.h"
#include "bar.h"
#include "wall.h"
#include "obstacle.h"
#include "button.h"
#include "physics.h"
#include "interface.h"
#include "interpreter.h"
#include "temporary_label.h"

// * * * * * * * * * * //
// Forward declarations

void glut_print (float x, float y, std::string s);
void display_fps(double dt);
void display_time();
void display_temperature(double temp);
void draw_particle(Particle& p);
void draw_bar(const Bar& b);
void draw_wall(const Wall& w);
void draw_vector(Vector2d v, Vector2d start, float r, float g, float b);
void draw_coords();
void draw_command_line();
void draw_button(const Button& b);
void draw_label(const TempLabel& l);
void draw_rectangle(Vector2d p1, Vector2d p2, bool filled);
void draw_circle(Vector2d centre, double r, unsigned int n_points);
void draw_cross(Vector2d pos, int size_px);
void draw_point(Vector2d pos);
void draw_horizon();

// * * * * * * * * * * //
vec3 hsv_to_rgb(vec3 hsv);

// * * * * * * * * * * //
int window_width = 1200;
int window_height = 800;
bool accelerations = false;
bool velocities = false;
bool lengths = false;
bool extensions = false;
bool coords = true;
bool ids = false;
bool fancy_bars = false;
bool show_particles = true;
bars_color_mode_t bars_color_mode = STRAIN_C;
const int wall_lines_spacing = 12; // px

// * * * * * * * * * * //
// Return the coordinates of the visible world edges
double window_left()
{ return -window_width/(2.0*world.scale) + world.centre.x; }

double window_right()
{ return window_width/(2.0*world.scale) + world.centre.x; }

double window_bottom()
{ return -window_height/(2.0*world.scale) + world.centre.y; }

double window_top()
{ return window_height/(2.0*world.scale) + world.centre.y; }

// * * * * * * * * * * //
void glut_print (float x, float y, std::string s)
// Prints string at location (x,y) in a bitmap font
{
    glRasterPos2f(x, y);
    for (unsigned short i = 0; i < s.length(); i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s[i]);
}

void display_fps(double dt)
{
    glColor3f(WHITE);
    
    std::ostringstream s;
    s << "fps: " << int(1/dt);
    glut_print(-1 + px_to_ui_x(30), -1 + px_to_ui_y(20), s.str());
}

void display_time()
{
    glColor3f(WHITE);
    
    std::ostringstream s;
    s.precision(1);
    s << "Time: " << std::fixed << simulation_time/1000000.0 << " s";
    glut_print(0, -1 + px_to_ui_y(BOTTOM_MARGIN), s.str());
}

void display_temperature(double temp)
{    
    if (temp > MELTING_POINT)
        glColor3f(RED);
    else
        glColor3f(WHITE);
    
    std::ostringstream s;
    s.precision(5);
    s << "T = " << int(temp) << " K";
    glut_print(1 - px_to_ui_x(80), -1 + px_to_ui_y(BOTTOM_MARGIN), s.str());
}

void draw_particle(Particle& p)
{
    // Draw the trace if it is enabled
    glColor3f(GOLD);
    glLineWidth(1);
    
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < p.trace.size(); i++)
    {
        Vector2d t = p.trace.get(i);
        glVertex2f(t.x, t.y);
    }
    glEnd();
    
    // Particle's position
    Vector2d pos = p.position_;
    
    // If fixed
    if (p.fixed_)
    {
        glColor3f(RED);
        glPointSize(8);
        glBegin(GL_POINTS);
        glVertex2f(pos.x, pos.y);
        glEnd();
    }
    else if (show_particles)
    {
        glColor3f(WHITE);
        glPointSize(6);
        glBegin(GL_POINTS);
        glVertex2f(pos.x, pos.y);
        glEnd();
    }
    
    if (ids)
    {
        std::stringstream s;
        s << p.id_;
        
        // Add 5 pixels in eah direction
        glColor3f(GOLD);
        glut_print(pos.x + px_to_m(5.0), pos.y + px_to_m(5.0), s.str());
    }
}

void draw_bar(const Bar& b)
{
    int mult = 5;
    
    // Color bars according to their strain
    if (bars_color_mode == STRAIN_C)
    {
        // Relative extension
        double strain = b.get_strain();
        if (strain > 1.0)
            strain = 1.0;
        else if (strain < -1.0)
            strain = -1.0;
        
        if (strain > 0.0)
            glColor3f(1.0, 1.0 - mult * strain / MAX_STRAIN, 1.0 - mult * strain / MAX_STRAIN);
        else
            glColor3f(1.0 + mult * strain / MAX_STRAIN, 1.0, 1.0);
    }
    // Color bars according to their temperature
    // TODO: Color it appropriately: black-red-yellow-white
    else if (bars_color_mode == TEMP_C)
    {
        double temp_fraction = (b.get_temperature() - ROOM_TEMPERATURE) / (MELTING_POINT - ROOM_TEMPERATURE);
        if (temp_fraction > 0.0)
            glColor3f(1.0, 1.0 - temp_fraction, 1.0 - temp_fraction); // red
        else
            glColor3f(1.0 + temp_fraction, 1.0, 1.0); // cyan
        
        // TODO
        vec3 color;
        double temp_chunk = MELTING_POINT / 3.0;
        double temp = b.get_temperature();
        if (temp <= temp_chunk)
            color = vec3(temp / temp_chunk, 0, 0);
        else if (temp <= 2 * temp_chunk)
            color = vec3(1.0, (temp - temp_chunk) / temp_chunk, 0.0);
        else if (temp <= 3 * temp_chunk)
            color = vec3(1.0, 1.0, (temp - 2 * temp_chunk) / temp_chunk);
        else
            color = vec3(1.0, 1.0, 1.0);
        glColor3f(color.x, color.y, color.z);
    }
    
    Vector2d start = particles[b.p1_id].position_;
    Vector2d end = particles[b.p2_id].position_;
    Vector2d m_mid = 0.5 * (particles[b.p1_id].position_ + particles[b.p2_id].position_);
    
    if (!fancy_bars)
    {
        glLineWidth(2.0);
        
        glBegin(GL_LINES);
        glVertex2f(start.x, start.y);
        glVertex2f(end.x, end.y);
        glEnd();
    }
    
    else
    {
        glLineWidth(1.0);
        
        Vector2d p1_pos = particles[b.p1_id].position_;
        Vector2d p2_pos = particles[b.p2_id].position_;
        
        // Draw circles at the ends
        double b_radius = 0.02;
        draw_circle(p1_pos, b_radius, 20); // 0.2m
        draw_circle(p2_pos, b_radius, 20);
        
        Vector2d unit = (p2_pos - p1_pos).norm();
        Vector2d normal = Vector2d(-unit.y, unit.x);
        
        Vector2d p1 = p1_pos + b_radius * normal;
        Vector2d p2 = p2_pos + b_radius * normal;
        Vector2d p3 = p1_pos - b_radius * normal;
        Vector2d p4 = p2_pos - b_radius * normal;
        
        glBegin(GL_LINES);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
        glVertex2f(p3.x, p3.y);
        glVertex2f(p4.x, p4.y);
        glEnd();
    }
    
    std::stringstream s;
    s.precision(3);
    
    if (ids)
    {
        glColor3f(FUCHSIA);
        s << b.id_;
        glut_print(m_mid.x, m_mid.y, s.str());
    }
    if (lengths)
    {
        s << b.length();
        glut_print(m_mid.x, m_mid.y, s.str());
    }
    if (extensions)
    {
        s.str("");
        s << b.get_strain();
        glut_print(m_mid.x, m_mid.y - px_to_m(12.0), s.str());
    }
}

void draw_wall(const Wall& w)
{
    glColor3f(WHITE);
    glLineWidth(2.0);
    
    draw_rectangle(w.p1_, w.p2_, false);
    
    double x_min = w.x_min();
    double x_max = w.x_max();
    double y_min = w.y_min();
    double y_max = w.y_max();
    
    double width = x_max - x_min;
    double height = y_max - y_min;
    
    // Spacing between the lines, metres
    double d = px_to_m(wall_lines_spacing);
    
    glBegin(GL_LINES);
    for (int i = 0; i * d <= width + height; i++)
    {
        double x1 = x_min + i * d - height;
        double y1 = y_min;
        double x2 = x_min + i * d;
        double y2 = y_max;
        
        if (x1 < x_min)
        {
            x1 = x_min;
            y1 = y_max - i * d;
        }
        if (x2 > x_max)
        {
            x2 = x_max;
            y2 = y_min + width + height - i * d;
        }
        
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        
    }
    glEnd();
    
    if (ids)
    {
        std::ostringstream s;
        s << w.id_;
        glColor3f(AQUA);
        glut_print(w.p2_.x, w.p2_.y, s.str());
    }
}

void draw_vector(Vector2d v, Vector2d start, float r, float g, float b)
{
    Vector2d end = start + v;
    
    glColor3f(r, g, b);
    glLineWidth(1.0);
    
    glBegin(GL_LINES);
    glVertex2f(start.x, start.y);
    glVertex2f(end.x, end.y);
    glEnd();
    
    glPointSize(5);
    
    glBegin(GL_POINTS);
    glVertex2f(end.x, end.y);
    glEnd();
}

void draw_coords()
{
    double left = window_left();
    double right = window_right();
    double bottom = window_bottom();
    double top = window_top();
    
    // Draw the centre lines
    glColor3f(DARK_GREY);
    glLineWidth(2.0);
    
    glBegin(GL_LINES);
    glVertex2f(0, window_bottom());
    glVertex2f(0, window_top());
    glVertex2f(window_left(), 0);
    glVertex2f(window_right(), 0);
    glEnd();
    
    glColor3f(DARK_GREY);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    
    double m_dist = px_to_m(grid_dist_px); // Distance between lines in metres
    
    // TODO: Clean this up
    
    // For +ve y
    for (int i = 0; i * m_dist < top; i++)
    {
        double y_pos = i * m_dist;
        glVertex2f(left, y_pos);
        glVertex2f(right, y_pos);
    }
    // For -ve y
    for (int i = -1; i * m_dist > bottom; i--)
    {
        double y_pos = i * m_dist;
        glVertex2f(left, y_pos);
        glVertex2f(right, y_pos);
    }
    // For +ve x
    for (int i = 0; i * m_dist < right; i++)
    {
        double x_pos = i * m_dist;
        glVertex2f(x_pos, bottom);
        glVertex2f(x_pos, top);
    }
    // For -ve x
    for (int i = -1; i * m_dist > left; i--)
    {
        double x_pos = i * m_dist;
        glVertex2f(x_pos, bottom);
        glVertex2f(x_pos, top);
    }
    
    glEnd();
    
    // Choose the right units to display
    double si_dist = 0.0;
    std::string unit;
    
    if (m_dist < 1e-3)
    {
        si_dist = m_dist * 1e6;
        unit = "um";
    }
    else if (m_dist < 1e-2)
    {
        si_dist = m_dist * 1e3;
        unit = "mm";
    }
    else if (m_dist < 1.0)
    {
        si_dist = m_dist * 1e2;
        unit = "cm";
    }
    else
    {
        si_dist = m_dist;
        unit = "m";
    }
    
    // Draw the scale (as a number)
    std::ostringstream s;
    s.precision(2);
    s << si_dist;
    glColor3f(GREY);
    glut_print(m_dist, 0.0, s.str() + unit);
}

void draw_command_line()
{
    double cmd_size = px_to_ui_y(COMMAND_LINE_SIZE);
    
    glColor3f(DARK_GREY);
    glBegin(GL_QUADS);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, -1 + cmd_size);
    glVertex2f(-1, -1 + cmd_size);
    glEnd();
    
    glColor3f(GOLD);
    glBegin(GL_LINES);
    glVertex2f(1, -1 + cmd_size);
    glVertex2f(-1, -1 + cmd_size);
    glEnd();
    
    glColor3f(WHITE);
    glut_print(-1 + px_to_ui_x(20), -1 + px_to_ui_y(10), commands[commands.size() - current_cmd - 1]);
}

void draw_button(const Button& b)
{
    Vector2d centre = b.position + px_to_ui(b.offset);
    Vector2d size = px_to_ui(Vector2d(b.width_/2.0, b.height_/2.0));
    glColor3f(DARK_GREY);
    draw_rectangle(centre - size, centre + size, true);
    
    if (b.active_)
    {
        glColor3f(YELLOW);
        glLineWidth(2.0);
    }
    else if (b.highlighted_)
    {
        glColor3f(GOLD);
        glLineWidth(2.0);
    }
    else
    {
        glColor3f(WHITE);
        glLineWidth(2.0);
    }
    draw_rectangle(centre - size, centre + size, false);
    
    glut_print(b.position.x + px_to_ui_x(b.offset.x - b.width_/2.0 + 6), b.position.y + px_to_ui_y(b.offset.y - 5), b.text_);
}

void draw_label(const TempLabel& l)
{
    glColor4f(1.0, 1.0, 1.0, l.alpha());
    if (l.centre)
        glut_print(l.position.x + px_to_ui_x(l.offset.x - l.text.size() * 2.8),
                   l.position.y + px_to_ui_y(l.offset.y - 6),
                   l.text);
    else
        glut_print(l.position.x + px_to_ui_x(l.offset.x),
                   l.position.y + px_to_ui_y(l.offset.y),
                   l.text);
}

void draw_rectangle(Vector2d p1, Vector2d p2, bool filled)
{
    if (!filled)
        glBegin(GL_LINE_LOOP);
    else
        glBegin(GL_QUADS);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p1.x, p2.y);
    glVertex2f(p2.x, p2.y);
    glVertex2f(p2.x, p1.y);
    glEnd();
}

// All in metres
void draw_circle(Vector2d centre, double r, unsigned int n_points)
{
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n_points; i++)
        glVertex2f(centre.x + r * cos(i * 2 * M_PI / n_points), centre.y + r * sin(i * 2 * M_PI / n_points));
    glEnd();
}

// Position in world coords, size in px
void draw_cross(Vector2d pos, int size_px)
{
    glBegin(GL_LINES);
    glVertex2f(pos.x - px_to_m(size_px), pos.y);
    glVertex2f(pos.x + px_to_m(size_px), pos.y);
    glVertex2f(pos.x, pos.y + px_to_m(size_px));
    glVertex2f(pos.x , pos.y - px_to_m(size_px));
    glEnd();
}

// * * * * * * * * * * //
// This is not currently used
vec3 hsv_to_rgb(vec3 hsv) // H is in the range [0,360] degs
{
    // TODO: check if given hsv is valid
    
    double C = hsv.z * hsv.y;
    double B = fmod((hsv.x/60.0), 2.0) - 1;
    double X = C * (1 - abs_d(B));
    double m = hsv.z - C;
    int n = (int)hsv.x / 60;
    
    vec3 rgb;
    if (n == 0)
        rgb = vec3(C, X, 0);
    else if (n == 1)
        rgb = vec3(X, C, 0);
    else if (n == 2)
        rgb = vec3(0, C, X);
    else if (n == 3)
        rgb = vec3(0, X, C);
    else if (n == 4)
        rgb = vec3(X, 0, C);
    else if (n == 5)
        rgb = vec3(C, 0, X);
    
    rgb.x += m;
    rgb.y += m;
    rgb.z += m;
    return rgb;
}

void draw_point(Vector2d pos)
{
    glBegin(GL_POINTS);
    glVertex2f(pos.x, pos.y);
    glEnd();
}

// For debugging
void draw_horizon()
{
    glColor3f(WHITE);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2d(-HORIZON, -HORIZON);
    glVertex2d(HORIZON, -HORIZON);
    glVertex2d(HORIZON, HORIZON);
    glVertex2d(-HORIZON, HORIZON);
    glEnd();
}

// * * * * * * * * * * //
void display()
{
    // Clear the window
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(WORLD_VIEW);
    
    if (coords && simulation_is_paused())
        draw_coords();
    
    // Draw the walls
    SlotMap<Wall>::iterator walls_it;
    for (walls_it = walls.begin(); walls_it != walls.end(); walls_it++)
        draw_wall(*walls_it);
    
    // Draw the obstacles
    SlotMap<Obstacle>::iterator o_it;
    for (o_it = obstacles.begin(); o_it != obstacles.end(); o_it++)
        o_it->draw();
    
    // Draw the bars
    SlotMap<Bar>::iterator bars_it;
    for (bars_it = bars.begin(); bars_it != bars.end(); bars_it++)
        draw_bar(*bars_it);
    
    // Draw the particles
    SlotMap<Particle>::iterator particles_it;
    for (particles_it = particles.begin(); particles_it != particles.end(); particles_it++)
        draw_particle(*particles_it);
    
    // Draw the velocity vectors
    if (velocities)
        for (particles_it = particles.begin(); particles_it != particles.end(); particles_it++)
            if (!particles_it->fixed_)
                draw_vector(particles_it->velocity_, particles_it->position_, 0.0, 0.5, 0.0);
    
    // Draw the acceleration vectors
    if (accelerations)
        for (particles_it = particles.begin(); particles_it != particles.end(); particles_it++)
            if (!particles_it->fixed_)
                draw_vector(particles_it->acceleration_, particles_it->position_, 0.0, 0.5, 0.0);
    
    // Draw the tool-specific things
    current_tool->display();
    
    // Switch to the UI view
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(UI_VIEW);
    
    // Draw temporary labels
    SlotMap<TempLabel>::iterator label_it;
    for (label_it = temp_labels.begin(); label_it != temp_labels.end(); label_it++)
        draw_label(*label_it);
    
    // Draw buttons
    for (int i = 0; i < buttons.size(); i++)
        draw_button(buttons[i]);
    
    display_temperature(environment_temp);
    display_time();
    
    // Draw the command line
    if (command_mode)
        draw_command_line();
    
    glutSwapBuffers();
}

void reshape(int width, int height)
{
    // Define the viewport transformation
    glViewport(0, 0, width, height);
    
    // Set the aspect ratio of the clipping area to match the viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(WORLD_VIEW);
    
    window_width = width;
    window_height = height;
}

void set_bars_color_mode(bars_color_mode_t mode)
{
    bars_color_mode = mode;
}

// * * * * * * * * * * //
void setup_graphics(int argc, char * argv[])
{
    // Initiallize GLUT
    glutInit(&argc, argv);
    
    // Setup for the new window
    glutInitWindowPosition(160, 80);
    glutInitWindowSize(window_width, window_height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    
    // Create a window
    glutCreateWindow("Trusses simulation");
    
    // Register callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glPointSize(5.0);
    glLineWidth(1.0);
    glClearColor(0.12, 0.12, 0.12, 1.0);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

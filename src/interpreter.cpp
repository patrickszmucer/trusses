//
//  interpreter.cpp
//  Trusses
//
//  Created by Patrick Szmucer on 20/01/2015.
//  Copyright (c) 2015 Patrick Szmucer. All rights reserved.
//

#include "interpreter.h"

#include <sstream>
#include <vector>

#include "particle.h"
#include "bar.h"
#include "save.h"
#include "temporary_label.h"
#include "window.h"
#include "settings.h"
#include "game.h"

using namespace std;

// * * * * * * * * * * //
template <typename T>
void Interpreter::extract(const string& str, vector<T> & target_v) const
{
    // Convert the string to the stringstream
    istringstream s(str);
    
    // Read every "word" of a stringstream
    while (s)
    {
        // If s is of type string
        T n;
        if (s >> n)
            target_v.push_back(n);
    }
}

std::string Interpreter::types_of_words(const std::vector<std::string>& words) const
{
    string types = "";
    for (int i = 0; i < words.size(); i++)
    {
        // Number
        if (is_number(words[i]))
            types += 'n';
        // Word
        else
            types += 'w';
    }
    return types;
}

bool Interpreter::is_number(const string& input)
{
    stringstream s(input);
    double n;
    if (s >> n)
        return true;
    else
        return false;
}

template <typename T>
T Interpreter::get_number(const string& input)
{
    stringstream s(input);
    T n;
    if (s >> n)
        return n;
    return 0;
}

bool Interpreter::is_bool(const std::string& input)
{
    return (input == "True" || input == "true" || input == "1" ||
            input == "False" || input == "false" || input == "0");
}

bool Interpreter::get_bool(const std::string &input)
{
    if (input == "True" || input == "true" || input == "1")
        return true;
    else
        return false;
}

void Interpreter::interpret() const
{
    // Put all the words of this command into the "words" vector of strings
    vector<string> words;
    extract<string>(command, words);
    size_t words_number = words.size();
    string types = types_of_words(words);
    
    // If no command given, return
    if (words_number == 0)
        return;
    
    string first_word = words[0];
    
    if (first_word == "ids")
    {
        if (words_number == 2 && words[1] == "on")
            settings.set(IDS, true);
        else if (words_number == 2 && words[1] == "off")
            settings.set(IDS, false);
        else
            issue_label("Usage: ids <on/off>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "gravity")
    {
        if (words_number == 2 && words[1] == "on")
            settings.set(GRAVITY, true);
        else if (words_number == 2 && words[1] == "off")
            settings.set(GRAVITY, false);
        else
            issue_label("Usage: gravity <on/off>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "grid")
    {
        if (words_number == 2 && words[1] == "on")
            settings.set(GRID, true);
        else if (words_number == 2 && words[1] == "off")
            settings.set(GRID, false);
        else
            issue_label("Usage: grid <on/off>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "load")
    {
        if (words_number == 2)
        {
            string filepath = words[1];
            
            // Check if the path is absolute or relative.
            // If it is relative, assume that it means the save directory.
            if (filepath.find("/") == -1)
                filepath = settings.get(SAVE_PATH) + filepath;
            
            // Try to load the file
            if (load(filepath))
                issue_label("Could not load " + filepath, WARNING_LABEL_TIME);
        }
        else
            issue_label("Usage: load <file>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "save")
    {
        if (words_number == 1)
        {
            string path = settings.get(SAVE_PATH);
            path += "save-" + date_str() + '-' + time_str();
            save(path);
        }
        else if (words_number == 2)
        {
            string path = settings.get(SAVE_PATH);
            path += words[1]; // TODO: SECURITY: Check if the filaname is valid
            save(path);
        }
        else
            issue_label("Usage: save <filename>", INFO_LABEL_TIME);
    }
    
    // Reset
    else if (first_word == "reset")
    {
        if (words_number == 1)
            game.reset();
        else
            issue_label("Usage: reset", INFO_LABEL_TIME);
    }
    
    // TODO: Maybe scale should be just an int?
    else if (first_word == "scale")
    {
        if (words_number == 1)
            cout << "scale=" << window.get_scale() << endl;
        else if (types == "wn")
        {
            double new_scale = get_number<double>(words[1]);
            if (new_scale <= 0.0)
                issue_label("Scale has to be positive", WARNING_LABEL_TIME);
            else
                window.set_scale(new_scale);
        }
        else
            issue_label("Usage: scale <double>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "particle")
    {
        if (words_number == 1)
            print_particles();
        else if (types == "wnn")
            Particle::create(get_number<double>(words[1]),
                            get_number<double>(words[2]), false);
        else
            issue_label("Usage: particle <double> <double>", INFO_LABEL_TIME);
    }
    
    // TODO: accept only positive integers
    else if (first_word == "bar")
    {
        if (words_number == 1)
            print_bars();
        else if (types == "wnn") // TODO: Prevent it from taking doubles
        {
            // Accept only positive integers
            // Interpret negative numbers as 0
            int n1 = get_number<int>(words[1]);
            int n2 = get_number<int>(words[2]);
            if (n1 < 0)
                n1 = 0;
            if (n2 < 0)
                n2 = 0;
            Bar::create(n1, n2);
        }
        else
            issue_label("Usage: bar <int> <int>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "fix")
    {
        if (types == "wn")
        {
            // Interpret negative numbers to 0
            int n = get_number<int>(words[1]);
            if (n < 0)
                n = 0;
            if (particles.exists(n))
                particles[n].fixed_ = true;
        }
        else
            issue_label("Usage: fix <particle id>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "trace")
    {
        if (types == "wn")
        {
            // Interpret negative numbers to 0
            int n = get_number<int>(words[1]);
            if (n < 0)
                n = 0;
            if (particles.exists(n))
                particles[n].trace();
        }
        else
            issue_label("Usage: trace <particle id>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "untrace")
    {
        if (types == "wn")
        {
            // Interpret negative numbers to 0
            int n = get_number<int>(words[1]);
            if (n < 0)
                n = 0;
            if (particles.exists(n))
                particles[n].untrace();
        }
        else
            issue_label("Usage: untrace <particle id>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "remove")
    {
        if (types == "wwn" && words[1] == "bar")
        {
            // Interpret negative numbers as 0
            int n = get_number<int>(words[2]);
            if (n < 0)
                n = 0;
            Bar::destroy(n);
        }
        else if (types == "wwn" && words[1] == "particle")
        {
            // Interpret negative numbers tas 0
            int n = get_number<int>(words[2]);
            if (n < 0)
                n = 0;
            Particle::destroy(n);
        }
        else
            issue_label("Usage: remove bar/particle <id>", INFO_LABEL_TIME);
    }
    
    // TODO: accept only positive integers
    else if (first_word == "split")
    {
        if (types == "wnn") // TODO: Prevent it from taking doubles
        {
            // Accept only positive integers
            // Interpret negative numbers as 0
            int n1 = get_number<int>(words[1]);
            int n2 = get_number<int>(words[2]);
            if (n1 < 0)
                n1 = 0;
            if (n2 < 0)
                n2 = 0;
            if (bars.exists(n1))
                bars[n1].split(n1, n2);
            else
                issue_label("This bar does not exist", WARNING_LABEL_TIME);
        }
        else
            issue_label("Usage: split <bar id> <number>", INFO_LABEL_TIME);
    }
    
    else if (first_word == "strain")
    {
        if (types == "wnn") // TODO: Prevent it from taking doubles
        {
            // Accept only positive integers
            // Interpret negative numbers as 0
            int n1 = get_number<int>(words[1]);
            double n2 = get_number<double>(words[2]);
            if (n1 < 0)
                n1 = 0;
            if (bars.exists(n1))
                bars[n1].set_strain(n2);
        }
        else
            issue_label("Usage: strain <bar id> <value>", INFO_LABEL_TIME);
    }
    
    // The command was not recognised
    else
        issue_label("Command not found", WARNING_LABEL_TIME);
}
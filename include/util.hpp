#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdarg>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>

using std::string;
// taken from pacman
#define _(str) (char *)str

// original on https://gitlab.archlinux.org/pacman/pacman/-/blob/master/src/pacman/conf.c#L45 
#define NOCOLOR       "\033[0m"

#define BOLD          "\033[0;1m"

#define BLACK         "\033[0;30m"
#define RED           "\033[0;31m"
#define GREEN         "\033[0;32m"
#define YELLOW        "\033[0;33m"
#define BLUE          "\033[0;34m"
#define MAGENTA       "\033[0;35m"
#define CYAN          "\033[0;36m"
#define WHITE         "\033[0;37m"

#define BOLDBLACK     "\033[1;30m"
#define BOLDRED       "\033[1;31m"
#define BOLDGREEN     "\033[1;32m"
#define BOLDYELLOW    "\033[1;33m"
#define BOLDBLUE      "\033[1;34m"
#define BOLDMAGENTA   "\033[1;35m"
#define BOLDCYAN      "\033[1;36m"
#define BOLDWHITE     "\033[1;37m"
#define GREY46        "\033[38;5;243m"

enum log_level {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO
};

template<typename T>
struct is_string {
    static constexpr bool value = std::is_same_v<T, string> || std::is_convertible_v<T, string>;
};

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
bool hasEnding(string const& fullString, string const& ending);
bool hasStart(string const& fullString, string const& start);
void log_printf(int log, string fmt, ...);
string expandVar(string& str);
string sanitizeStr(string& str);
std::vector<string> split(string text, char delim);


template <typename T>
T sanitize(T beg, T end) {
    T dest = beg;
    for (T itr = beg; itr != end; ++itr)
        // if its:
        // 1. a digit
        // 2. an uppercase letter
        // 3. a lowercase letter
        if (!(((*itr) >= 48 && (*itr) <= 57) && ((*itr) >= 65 && (*itr) <= 90) && ((*itr) >= 97 && (*itr) <= 122)))
            *(dest++) = *itr;
    return dest;
}

static inline const std::vector<string> secret = {
    {"Ingredients:"},
    {R"#(
    3/4 cup milk
    1/2 cup vegetable oil
    1 large egg
    2 cups all-purpose flour
    1/2 cup white sugar
    2 teaspoons baking powder
    1/2 teaspoon salt
    3/4 cup mini semi-sweet chocolate chips
    1 and 1/2 tablespoons white sugar
    1 tablespoon brown sugar
    )#"},
    {R"#(Step 1:
    Preheat the oven to 400 degrees F (200 degrees C).
    Grease a 12-cup muffin tin or line cups with paper liners.
    )#"},
    {R"#(Step 2:
    Combine milk, oil, and egg in a small bowl until well blended. 
    Combine flour, 1/2 cup sugar, baking powder, and salt together in a large bowl, making a well in the center. 
    Pour milk mixture into well and stir until batter is just combined; fold in chocolate chips.
    )#"},
    {R"#(Step 3:
    Spoon batter into the prepared muffin cups, filling each 2/3 full. 
    Combine 1 and 1/2 tablespoons white sugar and 1 tablespoon brown sugar in a small bowl; sprinkle on tops of muffins.
    )#"},
    {R"#(Step 4:
    Bake in the preheated oven until tops spring back when lightly pressed, about 18 to 20 minutes. 
    Cool in the tin briefly, then transfer to a wire rack.
            
            Serve warm or cool completely.
    )#"},
    {"Credits to: https://www.allrecipes.com/recipe/7906/chocolate-chip-muffins/"}
};

#endif

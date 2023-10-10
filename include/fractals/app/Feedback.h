#pragma once

#include <string>

#include <ftxui/screen/color.hpp>

namespace fractals::app {
    /**
    UI feedback severity level.
    */
    enum class FeedbackType { Error, Warning, Info };

    /**
    Feedback data type consists of severity level and message 
    */
    struct Feedback {
        FeedbackType m_type;
        std::string m_msg;
    };

    /**
    Color in which feedback message should be displayed.
    */
    ftxui::Color feedBackTypeColor(FeedbackType fbt);
    ftxui::Color feedBackColor(Feedback fb);

}
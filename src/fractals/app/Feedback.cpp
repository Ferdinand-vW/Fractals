#include <ftxui/screen/color.hpp>

#include "fractals/app/Feedback.h"

namespace fractals::app {
    using namespace ftxui;

    Color feedBackTypeColor(FeedbackType fbt) {
        switch(fbt) {
            case FeedbackType::Error: 
                return Color::Red;
            case FeedbackType::Warning:
                return Color::Yellow;
            case FeedbackType::Info:
                return Color::White;
            default:
                return Color::Blue;
        }
    }

    Color feedBackColor(Feedback fb) {
        return feedBackTypeColor(fb.m_type);
    }

}
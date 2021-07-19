#pragma once

#include <string>

#include <ftxui/screen/color.hpp>

namespace fractals::app {
    enum class FeedbackType { Error, Warning, Info };

    struct Feedback {
        FeedbackType m_type;
        std::wstring m_msg;
    };

    ftxui::Color feedBackTypeColor(FeedbackType fbt);
    ftxui::Color feedBackColor(Feedback fb);

}
#pragma once

#include <string>

#include <ftxui/screen/color.hpp>

using namespace ftxui;

enum class FeedbackType { Error, Warning, Info };

struct Feedback {
    FeedbackType m_type;
    std::wstring m_msg;
};

Color feedBackTypeColor(FeedbackType fbt);
Color feedBackColor(Feedback fb);
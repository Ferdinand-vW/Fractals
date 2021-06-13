#include "app/Feedback.h"

Color feedBackTypeColor(FeedbackType fbt) {
    switch(fbt) {
        case FeedbackType::Error: 
            return Color::Red;
        case FeedbackType::Warning:
            return Color::Yellow;
        case FeedbackType::Info:
            return Color::White;
    }
}

Color feedBackColor(Feedback fb) {
    return feedBackTypeColor(fb.m_type);
}
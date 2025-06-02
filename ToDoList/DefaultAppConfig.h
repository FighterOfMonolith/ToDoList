#pragma once

#include "Windows.h"
#include <string>

#define MAX_EVENTS 1000
#define MAX_EVENT_CATEGORIES 100

#define MAX_EVENT_CATEGORY_LENGTH 32
#define MAX_EVENT_NAME_LENGTH 24
#define MAX_EVENT_DESC_LENGTH 1024
#define MAX_EVENT_NOTES_LENGTH 256
#define MAX_AUTO_UPDATE_INTERVAL_SEC 86400
#define DEFAULT_AUTO_UPDATE_INTERVAL_SEC 5

#define MAX_SEARCH_INPUT_LENGTH MAX_EVENT_NAME_LENGTH

// Colors 

#define EVENT_PRIORITY_CRITICAL_COLOR		RGB(255, 140, 0)
#define EVENT_PRIORITY_HIGH_COLOR			RGB(255, 255, 0)
#define EVENT_PRIORITY_MEDIUM_COLOR			RGB(255, 255, 204)
#define EVENT_PRIORITY_LOW_COLOR			RGB(204, 209, 184)
#define EVENT_DONE_COLOR					RGB(187, 224, 182)
#define EVENT_PASTDUE_COLOR					RGB(230, 161, 161)


extern const std::wstring AboutAppMessage;

// Default categories to use if loading from file fails
extern const WCHAR* DEFAULT_CATEGORIES[];
extern const int DEFAULT_CATEGORY_COUNT;

// Styles 

extern const int DEFAULT_BUTTON_WIDTH;
extern const int DEFAULT_BUTTON_HEIGHT;
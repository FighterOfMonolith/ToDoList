#pragma once

#define UNICODE
#define _UNICODE

// Color definitions
#define COLOR_PRIORITY_CRITICAL      0x000000FF  // Red
#define COLOR_PRIORITY_HIGH          0x000080FF  // Orange
#define COLOR_PRIORITY_MEDIUM        0x0000FFFF  // Yellow
#define COLOR_PRIORITY_LOW           0x0000FF00  // Green
#define COLOR_PRIORITY_NONE          0x00C0C0C0  // Gray

#define COLOR_COMPLETED             0x0000FF00 
#define COLOR_OVERDUE               0x000000FF  
#define COLOR_IN_PROGRESS           0x08a8ff 

#include <Windows.h>
#include <string>
#include "Event.h"

class EventDetailsDialog
{
public:
    static const int LABEL_FONT_SIZE;
    static const int LABEL_FONT_WEIGHT;
    static const wchar_t* LABEL_FONT_FAMILY;
    static const Event* currentEvent;

    static void Show(HWND hParent, const Event* event);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, const Event* event);
    static std::wstring FormatEventDetails(const Event* event);
    static std::wstring GetPriorityString(EventPriority priority);
    static std::wstring GetRecurrenceString(RecurrenceInterval interval, int customDays);
    static COLORREF GetPriorityColor(EventPriority priority);
    static std::wstring GetStatusString(const Event* event);
    static void SetPriorityIndicatorColor(HWND hDlg, EventPriority priority);
    static void SetStatusIndicatorColor(HWND hDlg, const Event* event);
    static void UpdateTimeRemaining(HWND hDlg, const Event* event);
    static void UpdateStatusBar(HWND hStatusBar, const Event* event);
    static void CopyDetailsToClipboard(HWND hDlg, const Event* event);

};
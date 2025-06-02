#include "EventDetailsDialog.h"
#include "Windows.h"
#include "resource.h"
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "Utilities.h"

const int EventDetailsDialog::LABEL_FONT_SIZE = 18;
const int EventDetailsDialog::LABEL_FONT_WEIGHT = FW_NORMAL;
const wchar_t* EventDetailsDialog::LABEL_FONT_FAMILY = L"Arial Unicode MS";
const Event* EventDetailsDialog::currentEvent = nullptr;

void EventDetailsDialog::Show(HWND hParent, const Event* event)
{
    currentEvent = event;

    DialogBoxParam(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_EVENT_DETAILS_DIALOG),
        hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(event));
}

INT_PTR CALLBACK EventDetailsDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        OnInitDialog(hDlg, reinterpret_cast<const Event*>(lParam));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
        {
            // Clean up fonts
            HFONT hTitleFont = (HFONT)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            HFONT hLabelFont = (HFONT)GetWindowLongPtr(hDlg, GWLP_USERDATA + sizeof(LONG_PTR));
            DeleteObject(hTitleFont);
            DeleteObject(hLabelFont);

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        case IDC_INFO_COPY_DETAILS:
            CopyDetailsToClipboard(hDlg, reinterpret_cast<const Event*>(currentEvent));
            return TRUE;
        }
        break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        HWND hwnd = (HWND)lParam;

        if (hwnd == GetDlgItem(hDlg, IDC_EVENT_INFO_NAME))
        {
            SetTextColor(hdc, RGB(0, 0, 128)); // Dark blue for title
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
        }
        return FALSE;
    }

    case WM_DRAWITEM:
        if ((UINT)wParam == IDC_INFO_PRIORITY_INDICATOR)
        {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            Event* event = reinterpret_cast<Event*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

            HBRUSH hBrush = CreateSolidBrush(GetPriorityColor(event->priority));
            FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrush);
            DeleteObject(hBrush);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void EventDetailsDialog::OnInitDialog(HWND hDlg, const Event* event)
{
    // Store event pointer in window data
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)event);

    // Create fonts
    HFONT hTitleFont = CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    HFONT hLabelFont = CreateFont(LABEL_FONT_SIZE, 0, 0, 0, LABEL_FONT_WEIGHT, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, LABEL_FONT_FAMILY);

    // Set title with larger font
    SetWindowText(hDlg, (L"Деталі події: " + event->name).c_str());
    SendDlgItemMessage(hDlg, IDC_EVENT_INFO_NAME, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
    SetDlgItemText(hDlg, IDC_EVENT_INFO_NAME, (L"📌 " + event->name).c_str());

    // Set icon based on status
    HICON hIcon = LoadIcon(NULL, event->isCompleted ? IDI_INFORMATION :
        (event->isPastDeadline ? IDI_WARNING : IDI_QUESTION));
    SendDlgItemMessage(hDlg, IDC_EVENT_INFO_ICON, STM_SETICON, (WPARAM)hIcon, 0);

    // Apply label font to all static text controls (labels)
    EnumChildWindows(hDlg, [](HWND hWnd, LPARAM lParam) -> BOOL
        {
            wchar_t className[256];
            GetClassName(hWnd, className, 256);

            // Apply to static text controls that aren't the title
            if (wcscmp(className, L"Static") == 0 &&
                GetDlgCtrlID(hWnd) != IDC_EVENT_INFO_NAME)
            {
                SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            }
            return TRUE;
        }, (LPARAM)hLabelFont);

    // Set description
    SetDlgItemText(hDlg, IDC_EVENT_INFO_DESCRIPTION,
        event->description.empty() ? L"Опису немає" : event->description.c_str());

    // Set status
    SetDlgItemText(hDlg, IDC_EVENT_INFO_STATUS, GetStatusString(event).c_str());
    SetStatusIndicatorColor(hDlg, event);

    // Set priority
    SetDlgItemText(hDlg, IDC_EVENT_INFO_PRIORITY, GetPriorityString(event->priority).c_str());
    SetPriorityIndicatorColor(hDlg, event->priority);

    // Set dates
    SetDlgItemText(hDlg, IDC_EVENT_INFO_CREATION_DATE,
        (L"📅 " + SystemTimeToString(event->creationDate)).c_str());

    std::wstring deadlineText = L"⏰ " + SystemTimeToString(event->deadline);
    if (event->isPastDeadline && !event->isCompleted)
    {
        deadlineText += L" (прострочено)";
    }
    SetDlgItemText(hDlg, IDC_EVENT_INFO_DEADLINE, deadlineText.c_str());

    // Set category
    SetDlgItemText(hDlg, IDC_EVENT_INFO_CATEGORY, (L"📁 " + event->category).c_str());

    // Set recurrence
    if (event->isRecurring)
    {
        SetDlgItemText(hDlg, IDC_EVENT_INFO_RECURRENCE,
            (L"🔁 " + GetRecurrenceString(event->recurrenceInterval,
                event->customRecurrenceDays)).c_str());
    }
    else
    {
        SetDlgItemText(hDlg, IDC_EVENT_INFO_RECURRENCE, L"🚫 Без повторень");
    }

    // Set notes
    SetDlgItemText(hDlg, IDC_EVENT_INFO_NOTES,
        event->notes.empty() ? L"Нотаток немає" : event->notes.c_str());

    // Set time remaining
    UpdateTimeRemaining(hDlg, event);

    // Center dialog
    CenterWindow(hDlg);

    // Store fonts to delete them later
    SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)hTitleFont);
    SetWindowLongPtr(hDlg, GWLP_USERDATA + sizeof(LONG_PTR), (LONG_PTR)hLabelFont);
}

void EventDetailsDialog::SetStatusIndicatorColor(HWND hDlg, const Event* event)
{
    HWND hIndicator = GetDlgItem(hDlg, IDC_INFO_STATUS_INDICATOR);
    HDC hdc = GetDC(hIndicator);
    RECT rc;
    GetClientRect(hIndicator, &rc);

    HBRUSH hBrush;
    if (event->isCompleted)
    {
        hBrush = CreateSolidBrush(COLOR_COMPLETED); // Green
    }
    else if (event->isPastDeadline)
    {
        hBrush = CreateSolidBrush(COLOR_OVERDUE); // Red
    }
    else
    {
        hBrush = CreateSolidBrush(COLOR_IN_PROGRESS); // Blue
    }

    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);
    ReleaseDC(hIndicator, hdc);
}

void EventDetailsDialog::SetPriorityIndicatorColor(HWND hDlg, EventPriority priority)
{
    HWND hIndicator = GetDlgItem(hDlg, IDC_INFO_PRIORITY_INDICATOR);
    HDC hdc = GetDC(hIndicator);
    RECT rc;
    GetClientRect(hIndicator, &rc);

    HBRUSH hBrush = CreateSolidBrush(GetPriorityColor(priority));
    FillRect(hdc, &rc, hBrush);

    DeleteObject(hBrush);
    ReleaseDC(hIndicator, hdc);
}

void EventDetailsDialog::UpdateTimeRemaining(HWND hDlg, const Event* event)
{
    if (event->isCompleted)
    {
        SetDlgItemText(hDlg, IDC_INFO_TIME_REMAINING, L"✅ Подію виконано");
        return;
    }

    SYSTEMTIME now;
    GetLocalTime(&now);

    FILETIME ftNow, ftDeadline;
    SystemTimeToFileTime(&now, &ftNow);
    SystemTimeToFileTime(&event->deadline, &ftDeadline);

    ULARGE_INTEGER ulNow, ulDeadline;
    ulNow.LowPart = ftNow.dwLowDateTime;
    ulNow.HighPart = ftNow.dwHighDateTime;
    ulDeadline.LowPart = ftDeadline.dwLowDateTime;
    ulDeadline.HighPart = ftDeadline.dwHighDateTime;

    LONGLONG diff = ulDeadline.QuadPart - ulNow.QuadPart;

    if (diff <= 0)
    {
        SetDlgItemText(hDlg, IDC_INFO_TIME_REMAINING, L"❌ Час виконання події вже минув");
        return;
    }

    diff /= 10000000LL; // Convert to seconds

    int days = diff / 86400;
    int hours = (diff % 86400) / 3600;
    int minutes = (diff % 3600) / 60;

    std::wstring timeRemaining;
    if (days > 0)
    {
        timeRemaining = L"⏳ Залишилось: " + std::to_wstring(days) + L" дн. " +
            std::to_wstring(hours) + L" год.";
    }
    else if (hours > 0)
    {
        timeRemaining = L"⏳ Залишилось: " + std::to_wstring(hours) + L" год. " +
            std::to_wstring(minutes) + L" хв.";
    }
    else
    {
        timeRemaining = L"⏳ Залишилось: " + std::to_wstring(minutes) + L" хв.";
    }

    SetDlgItemText(hDlg, IDC_INFO_TIME_REMAINING, timeRemaining.c_str());
}

std::wstring EventDetailsDialog::GetStatusString(const Event* event)
{
    if (event->isCompleted)
    {
        return L"✅ Виконано вчасно";
    }
    return event->isPastDeadline ? L"❌ Прострочено" : L"🟢 Виконується";
}

COLORREF EventDetailsDialog::GetPriorityColor(EventPriority priority)
{
    switch (priority)
    {
    case EventPriority::Critical: return COLOR_PRIORITY_CRITICAL;
    case EventPriority::High:    return COLOR_PRIORITY_HIGH;
    case EventPriority::Medium:  return COLOR_PRIORITY_MEDIUM;
    case EventPriority::Low:     return COLOR_PRIORITY_LOW;
    default:                     return COLOR_PRIORITY_NONE;
    }
}

void EventDetailsDialog::UpdateStatusBar(HWND hStatusBar, const Event* event)
{
    std::wstring status = L"🔄 Статус: " + event->GetStatusString();
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)status.c_str());

    std::wstring priority = L"⚡ Пріоритет: " + GetPriorityString(event->priority);
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)priority.c_str());

    SYSTEMTIME now;
    GetLocalTime(&now);

    FILETIME ftNow, ftDeadline;
    SystemTimeToFileTime(&now, &ftNow);
    SystemTimeToFileTime(&event->deadline, &ftDeadline);

    ULARGE_INTEGER ulNow, ulDeadline;
    ulNow.LowPart = ftNow.dwLowDateTime;
    ulNow.HighPart = ftNow.dwHighDateTime;
    ulDeadline.LowPart = ftDeadline.dwLowDateTime;
    ulDeadline.HighPart = ftDeadline.dwHighDateTime;

    LONGLONG diff = ulDeadline.QuadPart - ulNow.QuadPart;
    diff /= 10000000LL; // Convert to seconds

    std::wstring timeRemaining;
    if (diff <= 0)
    {
        timeRemaining = L"Час вийшов";
    }
    else
    {
        int days = diff / 86400;
        int hours = (diff % 86400) / 3600;
        int minutes = (diff % 3600) / 60;

        if (days > 0)
        {
            timeRemaining = std::to_wstring(days) + L" днів " +
                std::to_wstring(hours) + L" год";
        }
        else if (hours > 0)
        {
            timeRemaining = std::to_wstring(hours) + L" год " +
                std::to_wstring(minutes) + L" хв";
        }
        else
        {
            timeRemaining = std::to_wstring(minutes) + L" хв";
        }
    }

    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)timeRemaining.c_str());
}

void EventDetailsDialog::CopyDetailsToClipboard(HWND hDlg, const Event* event)
{
    if (OpenClipboard(hDlg))
    {
        EmptyClipboard();

        std::wstring details = L"Подія: " + event->name + L"\r\n";
        details += L"Опис: " + event->description + L"\r\n";
        details += FormatEventDetails(event);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (details.size() + 1) * sizeof(wchar_t));
        if (hMem)
        {
            wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
            wcscpy_s(pMem, details.size() + 1, details.c_str());
            GlobalUnlock(hMem);

            SetClipboardData(CF_UNICODETEXT, hMem);
        }

        CloseClipboard();

        // Show feedback
        HWND hStatusBar = GetDlgItem(hDlg, IDC_EVENT_INFO_STATUS);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Деталі скопійовано в буфер обміну");
    }
}

std::wstring EventDetailsDialog::FormatEventDetails(const Event* event)
{
    std::wstringstream details;

    // Header
    details << L"════════════════════════════════════════\n";

    // Status with emoji and color coding
    details << L"🔄 Статус: ";
    if (event->isCompleted)
    {
        details << L"✅ Виконано";
    }
    else if (event->isPastDeadline)
    {
        details << L"❌ Не виконано (прострочено)";
    }
    else
    {
        details << L"🟢 Виконується";
    }
    details << L"\n\n";

    // Priority with colored indicator
    details << L"⚡ Пріоритет: " << GetPriorityString(event->priority) << L"\n\n";

    // Dates section
    details << L"📅 Дати:\n";
    details << L"   Створено: " << SystemTimeToString(event->creationDate) << L"\n";
    details << L"   Дедлайн:  " << SystemTimeToString(event->deadline) << L"\n\n";

    // Category
    details << L"📁 Категорія: " << event->category << L"\n\n";

    // Recurrence
    if (event->isRecurring)
    {
        details << L"🔁 Повторення: "
            << GetRecurrenceString(event->recurrenceInterval, event->customRecurrenceDays)
            << L"\n\n";
    }

    // Notes if available
    if (!event->notes.empty())
    {
        details << L"📝 Нотатки:\n" << event->notes << L"\n";
    }

    return details.str();
}

std::wstring EventDetailsDialog::GetPriorityString(EventPriority priority)
{
    switch (priority)
    {
    case EventPriority::Critical: return L"🔴 Критичний";
    case EventPriority::High:     return L"🔴 Високий";
    case EventPriority::Medium:   return L"🔴 Середній";
    case EventPriority::Low:      return L"⚪ Низький";
    default:                      return L"⚪ Не вказано";
    }
}

std::wstring EventDetailsDialog::GetRecurrenceString(RecurrenceInterval interval, int customDays)
{
    switch (interval)
    {
    case RecurrenceInterval::Daily:   return L"🔁 Щоденно";
    case RecurrenceInterval::Weekly:  return L"🔁 Щотижня";
    case RecurrenceInterval::Monthly: return L"🔁 Щомісяця";
    case RecurrenceInterval::Yearly:  return L"🔁 Щороку";
    case RecurrenceInterval::Custom:  return L"🔁 Кожні " + std::to_wstring(customDays) + L" днів";
    default:                          return L"🚫 Без повторень";
    }
}
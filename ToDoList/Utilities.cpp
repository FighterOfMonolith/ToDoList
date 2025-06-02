#define UNICODE
#define _UNICODE

#include "Utilities.h"
#include "AppState.h"
#include <sstream>
#include <iomanip>
#include <shlobj.h> // For SHBrowseForFolder

// Time-related utilities

std::wstring SystemTimeToString(const SYSTEMTIME& st)
{
    std::wstringstream ss;
    ss << std::setfill(L'0')
        << std::setw(2) << st.wDay << L"/"
        << std::setw(2) << st.wMonth << L"/"
        << std::setw(4) << st.wYear << L" "
        << std::setw(2) << st.wHour << L":"
        << std::setw(2) << st.wMinute;
    return ss.str();
}

SYSTEMTIME GetCurrentSystemTime()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    return st;
}

bool IsSystemTimeInPast(const SYSTEMTIME& st)
{
    SYSTEMTIME now;
    GetLocalTime(&now);
    return CompareSystemTimes(st, now) < 0;
}

int CompareSystemTimes(const SYSTEMTIME& a, const SYSTEMTIME& b)
{
    // Compare years
    if (a.wYear < b.wYear) return -1;
    if (a.wYear > b.wYear) return 1;

    // Years equal, compare months
    if (a.wMonth < b.wMonth) return -1;
    if (a.wMonth > b.wMonth) return 1;

    // Months equal, compare days
    if (a.wDay < b.wDay) return -1;
    if (a.wDay > b.wDay) return 1;

    // Days equal, compare hours
    if (a.wHour < b.wHour) return -1;
    if (a.wHour > b.wHour) return 1;

    // Hours equal, compare minutes
    if (a.wMinute < b.wMinute) return -1;
    if (a.wMinute > b.wMinute) return 1;

    // Minutes equal, compare seconds
    if (a.wSecond < b.wSecond) return -1;
    if (a.wSecond > b.wSecond) return 1;

    // Seconds equal, compare milliseconds
    if (a.wMilliseconds < b.wMilliseconds) return -1;
    if (a.wMilliseconds > b.wMilliseconds) return 1;

    // All fields equal
    return 0;
}

bool IsLeapYear(WORD year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DaysInMonth(WORD year, WORD month)
{
    static const int daysPerMonth[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month == 2 && IsLeapYear(year))
        return 29;

    return daysPerMonth[month - 1];
}

void NormalizeSystemTime(SYSTEMTIME* st)
{
    // Normalize milliseconds
    while (st->wMilliseconds >= 1000)
    {
        st->wMilliseconds -= 1000;
        st->wSecond++;
    }

    // Normalize seconds
    while (st->wSecond >= 60)
    {
        st->wSecond -= 60;
        st->wMinute++;
    }

    // Normalize minutes
    while (st->wMinute >= 60)
    {
        st->wMinute -= 60;
        st->wHour++;
    }

    // Normalize hours
    while (st->wHour >= 24)
    {
        st->wHour -= 24;
        st->wDay++;
    }

    // Normalize days
    while (true)
    {
        int dim = DaysInMonth(st->wYear, st->wMonth);
        if (st->wDay <= dim)
            break;

        st->wDay -= dim;
        st->wMonth++;
        if (st->wMonth > 12)
        {
            st->wMonth = 1;
            st->wYear++;
        }
    }

    // Normalize months
    while (st->wMonth > 12)
    {
        st->wMonth -= 12;
        st->wYear++;
    }
}

bool IsDueWithin(const SYSTEMTIME& deadline, int hours)
{
    if (hours <= 0) return false;

    SYSTEMTIME now;
    GetLocalTime(&now);

    // Convert both times to total minutes since epoch for easier comparison
    __int64 deadlineMinutes = SystemTimeToMinutes(deadline);
    __int64 nowMinutes = SystemTimeToMinutes(now);
    __int64 thresholdMinutes = deadlineMinutes - (hours * 60);

    return (nowMinutes >= thresholdMinutes) && (nowMinutes <= deadlineMinutes);
}

// Helper function to convert SYSTEMTIME to total minutes since year 1601
__int64 SystemTimeToMinutes(const SYSTEMTIME& st)
{
    SYSTEMTIME base = { 1601, 1, 0, 1, 0, 0, 0, 0 }; // Windows epoch
    FILETIME ftBase, ftCurrent;

    SystemTimeToFileTime(&base, &ftBase);
    SystemTimeToFileTime(&st, &ftCurrent);

    // Convert 100-nanosecond intervals to minutes
    ULARGE_INTEGER uliBase, uliCurrent;
    uliBase.LowPart = ftBase.dwLowDateTime;
    uliBase.HighPart = ftBase.dwHighDateTime;
    uliCurrent.LowPart = ftCurrent.dwLowDateTime;
    uliCurrent.HighPart = ftCurrent.dwHighDateTime;

    return (uliCurrent.QuadPart - uliBase.QuadPart) / (10000000 * 60);
}


// String utilities

std::wstring ToLower(const std::wstring& str)
{
    std::wstring lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

bool ContainsString(const std::wstring& str, const std::wstring& searchStr)
{
    return ToLower(str).find(ToLower(searchStr)) != std::wstring::npos;
}


// Event comparison utilities

bool CompareEventsByDeadline(const Event& a, const Event& b)
{
    return CompareSystemTimes(a.deadline, b.deadline) < 0;
}

bool CompareEventsByPriority(const Event& a, const Event& b)
{
    if (a.priority != b.priority)
        return a.priority > b.priority; // Higher priority first
    return CompareEventsByDeadline(a, b); // Then by deadline
}

bool CompareEventsByName(const Event& a, const Event& b)
{
    return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
}


// Get the executable folder path
std::wstring GetExecutableFolderPath()
{
    WCHAR path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH))
    {
        std::wstring executablePath = path;
        size_t lastSlashPos = executablePath.find_last_of(L"\\");
        if (lastSlashPos != std::wstring::npos)
        {
            return executablePath.substr(0, lastSlashPos);
        }
    }
    return L"";
}

// Helper method to get save file path
std::wstring GetSaveFilePath(HWND hWnd, const wchar_t* defaultExt, const wchar_t* filter)
{
    OPENFILENAME ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn))
    {
        return std::wstring(szFile);
    }
    return L"";
}

// Helper method to get open file path
std::wstring GetOpenFilePath(HWND hWnd, const wchar_t* defaultExt, const wchar_t* filter)
{
    OPENFILENAME ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        return std::wstring(szFile);
    }
    return L"";
}

// Helper method to select a folder
std::wstring GetFolderPath(HWND hWnd)
{
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hWnd;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL)
    {
        wchar_t path[MAX_PATH];
        SHGetPathFromIDList(pidl, path);

        // Free memory
        CoTaskMemFree(pidl);

        return std::wstring(path);
    }
    return L"";
}

// Convert UTF-8 or ANSI to UTF-16
std::wstring ConvertToWideStringFromUtf8OrAnsi(const char* buffer, size_t size)
{
    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buffer, size, NULL, 0);
    std::wstring result;

    if (required > 0)
    {
        result.resize(required);
        MultiByteToWideChar(CP_UTF8, 0, buffer, size, &result[0], required);
    }
    else
    {
        required = MultiByteToWideChar(CP_ACP, 0, buffer, size, NULL, 0);
        if (required > 0)
        {
            result.resize(required);
            MultiByteToWideChar(CP_ACP, 0, buffer, size, &result[0], required);
        }
    }

    return result;
}

// Trims whitespace from a string (both ends)
void TrimString(std::wstring& str)
{
    // Trim leading spaces
    size_t start = str.find_first_not_of(L" \t\r\n");
    if (start != std::wstring::npos)
    {
        str.erase(0, start);
    }

    // Trim trailing spaces
    size_t end = str.find_last_not_of(L" \t\r\n");
    if (end != std::wstring::npos)
    {
        str.erase(end + 1);
    }
}

std::wstring GetTranslatedErrorMessage(const std::exception& e)
{
    try
    {
        // For UTF-8 encoded exception messages
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(e.what());
    }
    catch (...)
    {
        // Fallback for non-UTF-8 strings
        int size_needed = MultiByteToWideChar(CP_ACP, 0, e.what(), -1, NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_ACP, 0, e.what(), -1, &wstrTo[0], size_needed);
        return wstrTo;
    }
}

int GetClientHeight(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    return rc.bottom - rc.top;
}

void CenterWindow(HWND hWnd)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);

    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate new position
    int x = (screenWidth - (rc.right - rc.left)) / 2;
    int y = (screenHeight - (rc.bottom - rc.top)) / 2;

    // Set new position
    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
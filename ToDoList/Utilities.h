#pragma once
#include <windows.h>
#include <string>
#include <algorithm>
#include "Event.h"

// Time-related utilities
std::wstring SystemTimeToString(const SYSTEMTIME& st);
SYSTEMTIME GetCurrentSystemTime();
bool IsSystemTimeInPast(const SYSTEMTIME& st);
int CompareSystemTimes(const SYSTEMTIME& a, const SYSTEMTIME& b);
bool IsLeapYear(WORD year);
int DaysInMonth(WORD year, WORD month);
void NormalizeSystemTime(SYSTEMTIME* st);
bool IsDueWithin(const SYSTEMTIME& time, int hours);
__int64 SystemTimeToMinutes(const SYSTEMTIME& st);

// String utilities
std::wstring ToLower(const std::wstring& str);
bool ContainsString(const std::wstring& str, const std::wstring& searchStr);

// Event comparison utilities
bool CompareEventsByDeadline(const Event& a, const Event& b);
bool CompareEventsByPriority(const Event& a, const Event& b);
bool CompareEventsByName(const Event& a, const Event& b);
std::wstring GetExecutableFolderPath();

std::wstring GetFolderPath(HWND hWnd);
std::wstring GetSaveFilePath(HWND hWnd, const wchar_t* defaultExt = L".txt", const wchar_t* filter = L"Txt Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
std::wstring GetOpenFilePath(HWND hWnd, const wchar_t* defaultExt = L".txt", const wchar_t* filter = L"Txt Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");

void TrimString(std::wstring& str);
std::wstring ConvertToWideStringFromUtf8OrAnsi(const char* buffer, size_t size);

std::wstring GetTranslatedErrorMessage(const std::exception& e);

int GetClientHeight(HWND hWnd);
void CenterWindow(HWND hWnd);
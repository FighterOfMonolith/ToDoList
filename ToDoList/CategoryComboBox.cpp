#include "CategoryComboBox.h"
#include <string>

HWND CategoryComboBox::Create(HWND parentHwnd, int x, int y, int width, int height, int controlId)
{
    // Create label
    CreateWindowW(
        L"STATIC",
        L"Категорія:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        x - 130, y, 120, 25,
        parentHwnd,
        NULL, NULL, NULL
    );

    // Create combobox
    HWND hCombo = CreateWindowW(
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL,
        x, y, width, 200,
        parentHwnd,
        (HMENU)controlId,
        NULL,
        NULL
    );

    // Initial population
    UpdateCategories(hCombo);

    return hCombo;
}

void CategoryComboBox::UpdateCategories(HWND comboBox)
{
    if (!comboBox) return;

    // Clear existing items
    SendMessageW(comboBox, CB_RESETCONTENT, 0, 0);

    // Add "All Categories" option
    SendMessageW(comboBox, CB_ADDSTRING, 0, (LPARAM)L"Всі категорії");

    // Load categories from AppConfig
    for (int i = 0; i < AppConfig::GetCategoryCount(); i++)
    {
        const WCHAR* category = AppConfig::GetCategory(i);
        SendMessageW(comboBox, CB_ADDSTRING, 0, (LPARAM)category);
    }

    // Select "All Categories" by default
    SendMessageW(comboBox, CB_SETCURSEL, 0, 0);
}

int CategoryComboBox::GetSelectedCategoryIndex(HWND comboBox)
{
    return (int)SendMessageW(comboBox, CB_GETCURSEL, 0, 0);
}

std::wstring CategoryComboBox::GetSelectedCategory(HWND comboBox)
{
    int index = GetSelectedCategoryIndex(comboBox);
    if (index <= 0) return L""; // "All Categories" or no selection

    WCHAR buffer[256] = L"\0";
    SendMessageW(comboBox, CB_GETLBTEXT, index, (LPARAM)buffer);
    return std::wstring(buffer);
}
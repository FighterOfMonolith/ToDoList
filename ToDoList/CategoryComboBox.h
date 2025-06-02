#pragma once
#include <Windows.h>
#include "AppConfig.h"

class CategoryComboBox
{
public:
    static HWND Create(HWND parentHwnd, int x, int y, int width, int height, int controlId);
    static void UpdateCategories(HWND comboBox);
    static int GetSelectedCategoryIndex(HWND comboBox);
    static std::wstring GetSelectedCategory(HWND comboBox);
};
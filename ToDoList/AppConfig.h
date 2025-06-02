#pragma once

#include "Windows.h"
#include <string>
#include "DefaultAppConfig.h"
#include <fstream>
#include <codecvt>

class AppConfig
{
public:
	static BOOL LoadConfig(); // Loads the configuration from file
	static BOOL SaveConfig(); // Saves the configuration to file
	static const WCHAR* GetCategory(int index);
	static int GetCategoryCount();
	static BOOL AddCategory(const WCHAR* newCategory);

private:
	static WCHAR** categories;
	static int categoryCount;
	static int maxCategories;

	static void ResizeCategoryArray();
};

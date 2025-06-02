#include "AppConfig.h"
#include "Utilities.h"

// Initialize static variables
WCHAR** AppConfig::categories = nullptr;
int AppConfig::categoryCount = 0;
int AppConfig::maxCategories = 0;

// Load configuration from a file
BOOL AppConfig::LoadConfig()
{
    std::wstring configPath = GetExecutableFolderPath() + L"\\app_config.txt";

    // Open the file with UTF-8 encoding
    std::wifstream configFile(configPath);

    if (!configFile.is_open())
    {
        // Failed to open file, use default categories
        categoryCount = 0;
        return FALSE;
    }

    // Set the locale to support UTF-8 encoded files
    configFile.imbue(std::locale(configFile.getloc(), new std::codecvt_utf8<wchar_t>));

    std::wstring line;
    categoryCount = 0;

    // Process the file line by line
    while (std::getline(configFile, line))
    {
        // Trim any extra whitespace from the line
        line.erase(std::remove(line.begin(), line.end(), L'\n'), line.end());
        line.erase(std::remove(line.begin(), line.end(), L'\r'), line.end());

        // Skip empty lines or comment lines (lines starting with "//")
        if (line.empty() || line.find(L"//") == 0)
            continue;

        // Handle the [Categories] section
        if (line.find(L"[Categories]") == 0)
        {
            // Read categories after the [Categories] header
            while (std::getline(configFile, line))
            {
                // Trim whitespace
                line.erase(std::remove(line.begin(), line.end(), L'\n'), line.end());
                line.erase(std::remove(line.begin(), line.end(), L'\r'), line.end());

                // Stop reading categories if we hit another section (e.g., [GeneralSettings])
                if (line.find(L"[") == 0)
                    break;

                // Each line should have the format "categoryN = categoryName"
                size_t eqPos = line.find(L"=");
                if (eqPos != std::wstring::npos)
                {
                    // Extract the category name after the "=" sign and trim spaces
                    std::wstring categoryName = line.substr(eqPos + 1);
                    categoryName.erase(std::remove(categoryName.begin(), categoryName.end(), L' '), categoryName.end());
                    categoryName.erase(std::remove(categoryName.begin(), categoryName.end(), L'\t'), categoryName.end());

                    // Add the category to the list if it�s not empty
                    if (!categoryName.empty() && categoryCount < MAX_EVENT_CATEGORIES)
                    {
                        AddCategory(categoryName.c_str());
                    }
                }
            }
        }
    }

    configFile.close();
    return TRUE;
}

// Save configuration to a file
BOOL AppConfig::SaveConfig()
{
    std::wstring configPath = GetExecutableFolderPath() + L"\\app_config.txt";

    // Open the file with UTF-8 encoding for writing (this will overwrite the file)
    std::wofstream configFile(configPath);
    if (!configFile.is_open())
    {
        return FALSE;
    }

    // Set the locale to support UTF-8 encoded files
    configFile.imbue(std::locale(configFile.getloc(), new std::codecvt_utf8<wchar_t>()));

    // Start by writing a header with comments about the config file
    configFile << L"// Application Configuration File\n";
    configFile << L"// This file contains categories and other settings for the application.\n";
    configFile << L"// You can edit these values, but please ensure the format is preserved.\n\n";

    // Write categories section with a comment
    configFile << L"[Categories]\n";
    for (int i = 0; i < categoryCount; i++)
    {
        configFile << L"category" << (i + 1) << L" = " << categories[i] << L"\n";
    }

    // If no categories are present, write a default message
    if (categoryCount == 0)
    {
        configFile << L"// No categories found. Please add some categories.\n";
    }

    // Close the file after writing the content
    configFile.close();
    return TRUE;
}


// Get category by index
const WCHAR* AppConfig::GetCategory(int index)
{
    if (index >= 0 && index < categoryCount)
    {
        return categories[index];
    }
    return L""; // Return empty string if invalid index
}

// Get the total count of categories
int AppConfig::GetCategoryCount()
{
    return categoryCount;
}

// Add category dynamically
BOOL AppConfig::AddCategory(const WCHAR* newCategory)
{
    // Trim leading and trailing whitespace from the category
    std::wstring trimmedCategory(newCategory);
    TrimString(trimmedCategory);

    // If the trimmed category is empty, return false
    if (trimmedCategory.empty())
    {
        return FALSE;
    }

    // Check for maximum category length
    if (wcslen(newCategory) >= MAX_EVENT_CATEGORY_LENGTH)
    {
        // If the category name exceeds the maximum length, return false
        return FALSE;
    }

    // Check if the category already exists
    for (int i = 0; i < categoryCount; i++)
    {
        if (wcscmp(categories[i], newCategory) == 0)
        {
            // Category already exists, so return false
            return FALSE;
        }
    }

    // Resize if needed
    if (categoryCount >= maxCategories)
    {
        ResizeCategoryArray();
    }

    // Copy the new category into the array
    size_t len = wcslen(newCategory) + 1; // Include the null terminator
    categories[categoryCount] = new WCHAR[len];
    wcscpy_s(categories[categoryCount], len, newCategory);

    categoryCount++;
    return TRUE; // Successfully added the category
}



// Resize the categories array when the number of categories exceeds the current size
void AppConfig::ResizeCategoryArray()
{
    // If it's the first allocation
    if (categories == nullptr)
    {
        maxCategories = 10; // Start with a smaller number
        categories = new WCHAR * [maxCategories];
    }
    else
    {
        // Double the array size when resizing
        maxCategories *= 2;
        WCHAR** newCategories = new WCHAR * [maxCategories];

        // Copy existing categories to the new array
        for (int i = 0; i < categoryCount; i++)
        {
            newCategories[i] = categories[i];
        }

        // Delete old array and update pointer
        delete[] categories;
        categories = newCategories;
    }
}

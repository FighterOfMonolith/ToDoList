#pragma once
#include <windows.h>
#include <commctrl.h>
#include "Event.h"
#include "AppConfig.h"
#include "Utilities.h"

class AppState
{
public:
    enum SortType
    {
        Name,
        Priority,
        Deadline,
        CreationDate,
    };

        enum FilterType
    {
        All,
        Active,
        Completed,
        PastDue
    };

    // Methods
    AppState();
    void Cleanup();

    bool AddEvent(Event* event);
    bool DeleteEvent(const Event* event);
    bool ToggleEventCompletion(const Event* event);
    Event* GetEvent(int index);
    void UpdateEvents();
    void ClearPastEvents();
    void SortEvents(SortType sortType);
    Event* GetAllEvents();
    int GetAllEventsCount();
    bool ContainsSameEvent(const Event* event);

    // Auto update settings
    void SetAutoUpdateEnabled(bool isEnabled);
    void SetAutoUpdateIntervalSec(int updateIntervalSec);
    int GetAutoUpdateIntervalSec();
    bool IsAutoUpdateEnabled();

    // Get filtered/search results
    void SearchEvents(const std::wstring& searchText);
    void FilterEvents(FilterType filterType);
    void FilterByCategory(const std::wstring& category);
    Event* GetFilteredEvents();
    int GetFilteredEventsCount() const;
    bool HasFilters() const;
    void ResetFilter();
    void ApplyCurrentFilter();
    Event* FindEventByGUID(const GUID& eventGUID);

    bool LoadEvents(const std::wstring& filePath);
    bool SaveEvents();
    bool SaveEventsAs(const std::wstring& filePath);

    bool SaveChanges();
    bool DiscardChanges();

private:

    // Event data
    Event* currentEvents = nullptr;
    Event* savedEvents = nullptr;

    int currentEventsCount = 0;
    int savedEventsCount = 0;
    int eventsCapacity = 0;

    // Filtered events
    Event* filteredEvents = nullptr;
    int filteredEventsCount = 0;
    int filteredEventsCapacity = 0;
    bool hasFilters = false;
    FilterType currentFilter = All;
    std::wstring currentSearchText;
    std::wstring currentCategoryFilter;

    // Settings
    bool autoUpdateEnabled;
    int autoUpdateInterval; // in minutes
    std::wstring currentSavePath;

    // Set default save path
    const std::wstring defaultEventsSavePath = GetExecutableFolderPath() + L"\\events.txt";

    // Metrics
    SYSTEMTIME lastMetricsUpdate;

    void ParseEventData(const std::wstring& key, const std::wstring& value, Event* currentEvent);
    bool ResizeEventArrays(int newCapacity);
    bool ResizeFilteredEventsArray(int newCapacity);
};
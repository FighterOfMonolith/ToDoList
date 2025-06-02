#include "AppState.h"
#include "Event.h"
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>
#include "Utilities.h"
#include <sstream>
#include "ApplicationUI.h"

#pragma region Initialization & Cleanup

AppState::AppState()
{
	currentEventsCount = 0;
	filteredEventsCapacity = 0;
	SetAutoUpdateEnabled(true);
	SetAutoUpdateIntervalSec(DEFAULT_AUTO_UPDATE_INTERVAL_SEC);
	ResizeEventArrays(4);

	// Initialize last metrics update to current time
	GetLocalTime(&lastMetricsUpdate);

	// Load categories from config file or use default
	if (!AppConfig::LoadConfig() || (AppConfig::GetCategoryCount() == 0))
	{
		for (int i = 0; i < DEFAULT_CATEGORY_COUNT; ++i)
		{
			AppConfig::AddCategory(DEFAULT_CATEGORIES[i]);
		}
	}

	if (!LoadEvents(defaultEventsSavePath))
	{
		// If loading failed, create empty events file
		try
		{
			// Create empty file
			FILE* file = NULL;
			if (_wfopen_s(&file, defaultEventsSavePath.c_str(), L"w,ccs=UTF-8") == 0 && file)
			{
				fclose(file);
				currentEventsCount = 0;
				currentSavePath = defaultEventsSavePath;
			}
			else
			{
				MessageBoxW(NULL,
					L"Не вдалося створити файл подій",
					L"Помилка",
					MB_ICONERROR);
			}
		}
		catch (...)
		{
			MessageBoxW(NULL,
				L"Помилка при створенні файлу подій",
				L"Помилка",
				MB_ICONERROR);
		}
	}

	UpdateEvents();
}

void AppState::Cleanup()
{
	AppConfig::SaveConfig();
	currentEventsCount = 0;
	savedEventsCount = 0;
	delete[] currentEvents;
	delete[] savedEvents;

	if (filteredEvents != nullptr)
	{
		delete[] filteredEvents;
		filteredEvents = nullptr;
	}
	filteredEventsCount = 0;
	filteredEventsCapacity = 0;
}

#pragma endregion


#pragma region Event Management

void AppState::SetAutoUpdateEnabled(bool isEnabled)
{
	autoUpdateEnabled = isEnabled;

	if (!autoUpdateEnabled)
	{
		SetAutoUpdateIntervalSec(DEFAULT_AUTO_UPDATE_INTERVAL_SEC);
	}
}

void AppState::SetAutoUpdateIntervalSec(int updateIntervalSec)
{
	if (updateIntervalSec > 0.1f)
	{
		autoUpdateInterval = updateIntervalSec;
	}
}

int AppState::GetAutoUpdateIntervalSec()
{
	return autoUpdateInterval;
}

bool AppState::IsAutoUpdateEnabled()
{
	return autoUpdateEnabled;
}

Event* AppState::GetAllEvents()
{
	return currentEvents;
}

int AppState::GetAllEventsCount()
{
	return currentEventsCount;
}

bool AppState::ContainsSameEvent(const Event* event)
{
	// Ensure currentEvents is not nullptr
	if (currentEvents == nullptr)
		return false;

	Event* currentEvent;

	for (int currentEventIndex = 0; currentEventIndex < currentEventsCount; ++currentEventIndex)
	{
		currentEvent = (currentEvents + currentEventIndex);

		if (currentEvent == event)
		{
			continue;
		}

		// Compare the event name
		if (currentEvent->name == event->name)
		{
			// Compare the deadline
			if (CompareSystemTimes(currentEvent->deadline, event->deadline) == 0)
			{
				// Event with same name and deadline found
				return true;
			}
		}
	}

	// No matching event found
	return false;

}

bool AppState::AddEvent(Event* event)
{
	if (currentEventsCount >= eventsCapacity)
	{
		if (!ResizeEventArrays(eventsCapacity * 2))
		{
			return false;
		}
	}

	currentEvents[currentEventsCount] = *event;
	currentEventsCount++;
	UpdateEvents();
	return true;
}

// Method to search an event by GUID
Event* AppState::FindEventByGUID(const GUID& eventGUID)
{
	for (int i = 0; i < currentEventsCount; ++i)
	{
		if (IsEqualGUID(currentEvents[i].uniqueID, eventGUID))
		{
			return &currentEvents[i];  // Return the pointer to the found event
		}
	}
	return nullptr;  // Return nullptr if event was not found
}

bool AppState::ToggleEventCompletion(const Event* event)
{
	Event* targetEvent = FindEventByGUID(event->uniqueID);

	if (targetEvent != nullptr && !targetEvent->isPastDeadline)
	{
		targetEvent->isCompleted = !targetEvent->isCompleted;
		return true;
	}
	return false;
}


bool AppState::DeleteEvent(const Event* event)
{
	// Find the event by GUID
	Event* targetEvent = FindEventByGUID(event->uniqueID);
	if (targetEvent == nullptr)
		return false;  // Event not found

	// Find the index of the event in currentEvents array
	int index = -1;
	for (int i = 0; i < currentEventsCount; ++i)
	{
		if (IsEqualGUID(currentEvents[i].uniqueID, event->uniqueID))
		{
			index = i;
			break;
		}
	}

	if (index == -1)
		return true;

	// Shift all elements after the found index one position left
	for (int i = index; i < currentEventsCount - 1; i++)
	{
		currentEvents[i] = currentEvents[i + 1];
	}

	currentEventsCount--;
	UpdateEvents();

	return true;
}


Event* AppState::GetEvent(int index)
{
	return &currentEvents[index];
}

bool AppState::ResizeEventArrays(int newCapacity)
{
	Event* newCurrent = new Event[newCapacity];
	Event* newSaved = new Event[newCapacity];

	if (!newCurrent || !newSaved)
	{
		delete[] newCurrent;
		delete[] newSaved;
		return false;
	}

	// Copy existing events
	for (int i = 0; i < currentEventsCount; i++)
	{
		newCurrent[i] = currentEvents[i];
	}

	for (int i = 0; i < savedEventsCount; i++)
	{
		newSaved[i] = savedEvents[i];
	}

	// Clean up old arrays
	delete[] currentEvents;
	delete[] savedEvents;

	// Assign new arrays
	currentEvents = newCurrent;
	savedEvents = newSaved;
	eventsCapacity = newCapacity;

	return true;
}

bool AppState::SaveChanges()
{
	// Ensure saved array has enough capacity
	if (savedEventsCount < currentEventsCount)
	{
		if (!ResizeEventArrays(max(eventsCapacity, currentEventsCount)))
		{
			return false;
		}
	}

	// Copy all current events to saved
	savedEventsCount = currentEventsCount;
	for (int i = 0; i < currentEventsCount; i++)
	{
		savedEvents[i] = currentEvents[i];
	}

	return true;
}

bool AppState::DiscardChanges()
{
	// Ensure current array has enough capacity
	if (currentEventsCount < savedEventsCount)
	{
		if (!ResizeEventArrays(max(eventsCapacity, savedEventsCount)))
		{
			return false;
		}
	}

	// Copy all saved events to current
	currentEventsCount = savedEventsCount;
	for (int i = 0; i < savedEventsCount; i++)
	{
		currentEvents[i] = savedEvents[i];
	}

	UpdateEvents();
	return true;
}

void AppState::UpdateEvents()
{
	SYSTEMTIME now;
	GetLocalTime(&now);

	// count how many new recurring events we'll need to create
	int newRecurringCount = 0;
	for (int i = 0; i < currentEventsCount; i++)
	{
		Event* event = &currentEvents[i];
		event->UpdateStatus();

		if (event->isRecurring && event->isPastDeadline &&
			!event->isCompleted && !event->isReccurenceHandled)
		{
			Event newEvent = *event;

			switch (event->recurrenceInterval)
			{
			case RecurrenceInterval::Daily:
				newEvent.deadline.wDay++;
				break;
			case RecurrenceInterval::Weekly:
				newEvent.deadline.wDay += 7;
				break;
			case RecurrenceInterval::Monthly:
				newEvent.deadline.wMonth++;
				break;
			case RecurrenceInterval::Yearly:
				newEvent.deadline.wYear++;
				break;
			case RecurrenceInterval::Custom:
				newEvent.deadline.wDay += event->customRecurrenceDays;
				break;
			default:
				break;
			}

			// Normalize the date
			NormalizeSystemTime(&newEvent.deadline);

			newEvent.isCompleted = false;
			newEvent.isPastDeadline = false;
			newEvent.isReccurenceHandled = false;
			CoCreateGuid(&newEvent.uniqueID);

			// Mark original as handled
			event->isReccurenceHandled = true;

			AddEvent(&newEvent);
		}
	}

	if (newRecurringCount == 0)
		return;

	Event* newEvents = new Event[currentEventsCount + newRecurringCount];
	int newEventsCount = 0;

	for (int i = 0; i < currentEventsCount; i++)
	{
		// Copy the original event
		newEvents[newEventsCount++] = currentEvents[i];
		Event* currentEvent = &newEvents[newEventsCount - 1];

		// Handle recurrence if needed
		if (currentEvent->isRecurring && currentEvent->isPastDeadline &&
			!currentEvent->isCompleted && !currentEvent->isReccurenceHandled)
		{
			Event newEvent = *currentEvent;


		}
	}

	// Replace old array with new one
	delete[] currentEvents;
	currentEvents = newEvents;
	currentEventsCount = newEventsCount;
	eventsCapacity = currentEventsCount;

	// Update filtered events if needed
	if (hasFilters)
	{
		ApplyCurrentFilter();
	}
}

void AppState::ClearPastEvents()
{
	int writeIndex = 0;
	for (int i = 0; i < currentEventsCount; i++)
	{
		if (!(currentEvents[i].isPastDeadline && !currentEvents[i].isCompleted))
		{
			currentEvents[writeIndex] = currentEvents[i];
			writeIndex++;
		}
	}
	currentEventsCount = writeIndex;
}

// Comparison functions for each sorting type
bool CompareByDeadline(const Event& a, const Event& b)
{
	return CompareSystemTimes(a.deadline, b.deadline) < 0;
}

bool CompareByPriority(const Event& a, const Event& b)
{
	return a.priority > b.priority; // Higher priority first
}

bool CompareByName(const Event& a, const Event& b)
{
	return a.name < b.name;
}

bool CompareByCreationDate(const Event& a, const Event& b)
{
	return CompareSystemTimes(a.creationDate, b.creationDate) < 0;
}

void AppState::SortEvents(SortType sortType)
{
	// Determine which array to sort and its size
	bool hasFilters = HasFilters();
	Event* arrayToSort = hasFilters ? filteredEvents : currentEvents;
	int count = hasFilters ? filteredEventsCount : currentEventsCount;

	if (count <= 1) return;

	// Sort the appropriate array using the comparison function
	switch (sortType)
	{
	case SortType::Deadline:
		std::sort(arrayToSort, arrayToSort + count, CompareByDeadline);
		break;
	case SortType::Priority:
		std::sort(arrayToSort, arrayToSort + count, CompareByPriority);
		break;
	case SortType::Name:
		std::sort(arrayToSort, arrayToSort + count, CompareByName);
		break;
	case SortType::CreationDate:
		std::sort(arrayToSort, arrayToSort + count, CompareByCreationDate);
		break;
	default:
		break;
	}
}

bool AppState::SaveEvents()
{
	if (!currentSavePath.empty())
	{
		return SaveEventsAs(currentSavePath);
	}
	return false;
}

bool AppState::SaveEventsAs(const std::wstring& filePath)
{
	FILE* file = NULL;
	errno_t err = _wfopen_s(&file, filePath.c_str(), L"w,ccs=UTF-8");
	if (err != 0 || !file)
	{
		MessageBoxW(NULL, L"Не вдалося створити файл для збереження", L"Помилка збереження", MB_ICONERROR);
		return false;
	}

	try
	{
		for (int i = 0; i < currentEventsCount; i++)
		{
			Event* event = &currentEvents[i];

			// Write event header
			if (fwprintf(file, L"[Event %d]\n", i + 1) < 0)
				throw std::runtime_error("Не вдалося записати заголовок події");

			// Write basic fields with error checking
			if (fwprintf(file, L"Name=%s\n", event->name.c_str()) < 0 ||
				fwprintf(file, L"Description=%s\n", event->description.c_str()) < 0 ||
				fwprintf(file, L"Notes=%s\n", event->notes.c_str()) < 0 ||
				fwprintf(file, L"Category=%s\n", event->category.c_str()) < 0)
			{
				throw std::runtime_error("Не вдалося записати текстові поля");
			}

			// Write boolean fields
			if (fwprintf(file, L"Completed=%s\n", event->isCompleted ? L"true" : L"false") < 0 ||
				fwprintf(file, L"PastDeadline=%s\n", event->isPastDeadline ? L"true" : L"false") < 0 ||
				fwprintf(file, L"Recurring=%s\n", event->isRecurring ? L"true" : L"false") < 0)
			{
				throw std::runtime_error("Не вдалося записати булеві поля");
			}

			// Write numeric fields
			if (fwprintf(file, L"Priority=%d\n", event->priority) < 0 ||
				fwprintf(file, L"RecurrenceInterval=%d\n", static_cast<int>(event->recurrenceInterval)) < 0 ||
				fwprintf(file, L"CustomRecurrenceDays=%d\n", event->customRecurrenceDays) < 0)
			{
				throw std::runtime_error("Не вдалося записати числові поля");
			}

			// Write dates with proper formatting
			if (fwprintf(file, L"CreationDate=%04d-%02d-%02d %02d:%02d\n",
				event->creationDate.wYear, event->creationDate.wMonth, event->creationDate.wDay,
				event->creationDate.wHour, event->creationDate.wMinute) < 0 ||
				fwprintf(file, L"Deadline=%04d-%02d-%02d %02d:%02d\n",
					event->deadline.wYear, event->deadline.wMonth, event->deadline.wDay,
					event->deadline.wHour, event->deadline.wMinute) < 0)
			{
				throw std::runtime_error("Не вдалося записати дати");
			}

			// Add separator
			if (fwprintf(file, L"\n") < 0)
				throw std::runtime_error("Не вдалось записати розділювач");
		}

		currentSavePath = filePath;
	}
	catch (const std::exception& e)
	{
		std::wstring prefix = L"Помилка під час зберігання подій у файл";
		std::wstring errorMsg = GetTranslatedErrorMessage(e);
		std::wstring fullMessage = prefix + errorMsg;

		MessageBoxW(NULL, fullMessage.c_str(), L"Помилка зберігання", MB_ICONERROR);
		if (file)
		{
			fclose(file);
			// Delete incomplete file
			_wremove(filePath.c_str());
		}
		return false;
	}

	SaveChanges();
	fclose(file);
	return true;
}

bool AppState::LoadEvents(const std::wstring& filePath)
{
	// Open file and read raw bytes
	FILE* file = NULL;
	if (_wfopen_s(&file, filePath.c_str(), L"rb") != 0 || !file)
	{
		return false;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (fileSize <= 0)
	{
		fclose(file);
		return false;
	}

	// Read the file into a buffer
	char* buffer = new char[fileSize];
	if (fread(buffer, 1, fileSize, file) != static_cast<size_t>(fileSize))
	{
		delete[] buffer;
		fclose(file);
		return false;
	}
	fclose(file);

	// Remove BOM if present (UTF-8 or UTF-16 BOM)
	std::wstring content;
	int encoding = IS_TEXT_UNICODE_UNICODE_MASK;
	size_t offset = 0;

	if (IsTextUnicode(reinterpret_cast<BYTE*>(buffer), fileSize, &encoding))
	{
		// UTF-16 file, no need for conversion
		if (fileSize > 2 && (buffer[0] == (char)0xFF && buffer[1] == (char)0xFE))  // UTF-16 BOM check
		{
			offset = 2; // Skip BOM
		}
		content.assign(reinterpret_cast<wchar_t*>(buffer + offset), (fileSize - offset) / sizeof(wchar_t));
	}
	else
	{
		// Handle UTF-8 and ANSI encodings
		content = ConvertToWideStringFromUtf8OrAnsi(buffer, fileSize);
	}

	// Clean up buffer
	delete[] buffer;

	// Remove BOM from content
	if (!content.empty() && (content[0] == L'\xFEFF' || content[0] == L'\xFFFE'))
	{
		content.erase(0, 1);  // Remove BOM
	}

	// Clear existing events
	currentEventsCount = 0;
	Event* currentEvent = nullptr;

	std::wistringstream iss(content);
	std::wstring line;

	while (std::getline(iss, line))
	{
		// Trim whitespace
		TrimString(line);

		if (line.empty()) continue;

		// New event section
		if (line[0] == L'[' && line.find(L"Event") != std::wstring::npos)
		{
			if (currentEventsCount >= MAX_EVENTS)
			{
				MessageBoxW(NULL, L"Максимальна кількість подій досягнута", L"Попередження", MB_ICONWARNING);
				break;
			}

			if (currentEventsCount >= eventsCapacity)
			{
				ResizeEventArrays(eventsCapacity * 2);
			}
			currentEvent = &currentEvents[currentEventsCount++];
			*currentEvent = Event(); // Reset to defaults
			continue;
		}

		if (!currentEvent) continue;

		// Parse key-value pair
		size_t eqPos = line.find(L'=');
		if (eqPos == std::wstring::npos) continue;

		std::wstring key = line.substr(0, eqPos);
		std::wstring value = line.substr(eqPos + 1);

		// Trim whitespace
		TrimString(key);
		TrimString(value);

		try
		{
			ParseEventData(key, value, currentEvent);
			if (currentEvent->isPastDeadline)
			{
				currentEvent->MarkNotified(NOTIFICATION_DEADLINE_PASSED);
			}

			if (key == L"Category")
			{
				if (AppConfig::AddCategory(currentEvent->category.c_str()))
				{
					ApplicationUserInterface::ApplicationUI::RefreshCategories();
					AppConfig::SaveConfig();
				}
			}
		}
		catch (const std::exception& e)  // Catch standard exceptions
		{
			// Display the exception message
			std::wstring errorPrefix = L"Помилка завантаження з файлу: ";
			std::wstring errorMessage = GetTranslatedErrorMessage(e);
			std::wstring fullMessage = errorPrefix + errorMessage;

			MessageBoxW(NULL, fullMessage.c_str(), L"Помилка завантаження", MB_ICONERROR);
			currentEventsCount = 0;
			return false;
		}
		catch (...)
		{
			// Catch any non-standard exception
			MessageBoxW(NULL, L"Помилка завантаження з файлу : неможливо виконати парсинг даних. Можливо файл пошкоджено.", L"Помилка завантаження", MB_ICONERROR);
			currentEventsCount = 0;
			return false;
		}
	}

	currentSavePath = filePath;
	SaveChanges();
	return true;
}

void AppState::ParseEventData(const std::wstring& key, const std::wstring& value, Event* currentEvent)
{
	if (key == L"Name")
	{
		if (value.length() > MAX_EVENT_NAME_LENGTH)
			throw std::runtime_error("Назва події перевищує максимальну довжину");
		currentEvent->name = value;
	}
	else if (key == L"Description")
	{
		if (value.length() > MAX_EVENT_DESC_LENGTH)
			throw std::runtime_error("Опис перевищує максимальну довжину");
		currentEvent->description = value;
	}
	else if (key == L"Notes")
	{
		if (value.length() > MAX_EVENT_NOTES_LENGTH)
			throw std::runtime_error("Примітки перевищують максимальну довжину");
		currentEvent->notes = value;
	}
	else if (key == L"Category")
	{
		if (value.length() > MAX_EVENT_CATEGORY_LENGTH)
			throw std::runtime_error("Категорія перевищує максимальну довжину");
		currentEvent->category = value;
	}
	else if (key == L"Completed")
	{
		currentEvent->isCompleted = (value == L"true");
	}
	else if (key == L"PastDeadline")
	{
		currentEvent->isPastDeadline = (value == L"true");
	}
	else if (key == L"Recurring")
	{
		currentEvent->isRecurring = (value == L"true");
	}
	else if (key == L"Priority")
	{
		int priority = _wtoi(value.c_str());
		if (priority < 0 || priority > 3) // Assuming priority range is 0-3
			throw std::runtime_error("Невірне значення пріоритету");
		currentEvent->priority = static_cast<EventPriority>(priority);
	}
	else if (key == L"RecurrenceInterval")
	{
		int interval = _wtoi(value.c_str());
		if (interval < 0 || interval > 4) // Assuming 0-4 for None, Daily, Weekly, Monthly, Yearly, Custom
			throw std::runtime_error("Невірне значення інтервалу повторення");
		currentEvent->recurrenceInterval = static_cast<RecurrenceInterval>(interval);
	}
	else if (key == L"CustomRecurrenceDays")
	{
		int days = _wtoi(value.c_str());
		if (days < 0 || days > 365 * 10) // Reasonable limit for custom days
			throw std::runtime_error("Невірне значення кількості днів для інтервалу повторення");
		currentEvent->customRecurrenceDays = days;
	}
	else if (key == L"CreationDate" || key == L"Deadline")
	{
		SYSTEMTIME* st = (key == L"CreationDate") ? &currentEvent->creationDate : &currentEvent->deadline;
		if (swscanf_s(value.c_str(), L"%04d-%02d-%02d %02d:%02d",
			&st->wYear, &st->wMonth, &st->wDay, &st->wHour, &st->wMinute) != 5)
		{
			throw std::runtime_error("Невірний формат дати");
		}

		// Additional date validation
		if (st->wYear < 1900 || st->wYear > 2100 ||
			st->wMonth < 1 || st->wMonth > 12 ||
			st->wDay < 1 || st->wDay > 31 ||
			st->wHour > 23 || st->wMinute > 59)
		{
			throw std::runtime_error("Невірні значення дати");
		}
	}
	else
	{
		throw std::runtime_error("Невідоме поле в даних події");
	}
}


#pragma endregion


#pragma region Filters

bool AppState::ResizeFilteredEventsArray(int newCapacity)
{
	if (newCapacity <= filteredEventsCapacity)
		return true;

	// Allocate a new array with the required capacity
	Event* newArray = new Event[newCapacity]; // A simple 1D array of Event
	if (!newArray)
		return false;

	// Copy the events from the old array to the new one
	for (int i = 0; i < filteredEventsCount; i++)
	{
		newArray[i] = filteredEvents[i];
	}

	// Delete the old array
	delete[] filteredEvents;
	filteredEvents = newArray;
	filteredEventsCapacity = newCapacity;
	return true;
}

// Get the filtered events (or all events if no filter is applied)
Event* AppState::GetFilteredEvents()
{
	return hasFilters ? filteredEvents : currentEvents;
}

// Get the number of filtered events (or total events if no filter is applied)
int AppState::GetFilteredEventsCount() const
{
	return hasFilters ? filteredEventsCount : currentEventsCount;
}

// Check if filtered results are being shown
bool AppState::HasFilters() const
{
	return hasFilters;
}

// Reset the filter and clear the search text
void AppState::ResetFilter()
{
	hasFilters = false;
	currentFilter = All;
	currentSearchText.clear();
	filteredEventsCount = 0;
}

// Apply the current filter and update the filtered events list
void AppState::FilterEvents(FilterType filterType)
{
	currentFilter = filterType;
	ApplyCurrentFilter();
}

// Search events based on the search text
void AppState::SearchEvents(const std::wstring& searchText)
{
	currentSearchText = searchText;
	ApplyCurrentFilter();
}

void AppState::FilterByCategory(const std::wstring& category)
{
	currentCategoryFilter = category;
	ApplyCurrentFilter();
}

// Apply the current filter and update the filtered events
void AppState::ApplyCurrentFilter()
{
	filteredEventsCount = 0;
	hasFilters = false;

	// Ensure we have enough capacity in the filtered events array
	if (filteredEventsCapacity < currentEventsCount)
	{
		if (!ResizeFilteredEventsArray(currentEventsCount + 10))
			return;
	}

	// Apply the filter based on the filter type
	for (int i = 0; i < currentEventsCount; i++)
	{
		Event* event = &currentEvents[i];
		bool matchesFilter = true;

		// Apply status filter
		switch (currentFilter)
		{
		case Active: matchesFilter &= !event->isCompleted && !event->isPastDeadline; break;
		case Completed: matchesFilter &= event->isCompleted; break;
		case PastDue: matchesFilter &= event->isPastDeadline && !event->isCompleted; break;
		case All: break;
		}

		// Apply search text filter
		if (!currentSearchText.empty())
		{
			matchesFilter &= (event->name.find(currentSearchText) != std::wstring::npos) ||
				(event->description.find(currentSearchText) != std::wstring::npos);
		}

		// Apply category filter
		if (!currentCategoryFilter.empty())
		{
			matchesFilter &= (event->category == currentCategoryFilter);
		}

		if (matchesFilter)
		{
			filteredEvents[filteredEventsCount++] = *event;
		}
	}

	// If filtered events exist, or there is a non-empty search text, or filter is not 'All', show filtered results

	hasFilters = (currentFilter != All) || (!currentSearchText.empty()) || (!currentCategoryFilter.empty());
}

#pragma endregion

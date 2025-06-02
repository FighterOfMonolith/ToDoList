
#define UNICODE
#define _UNICODE

#include "ApplicationUI.h"
#include "EventDialog.h"
#include <windowsx.h>
#include <string>
#include <algorithm>
#include "Utilities.h"
#include "resource.h"
#include "EventList.h"
#include "CategoryComboBox.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace ApplicationUserInterface
{
	AppState* ApplicationUI::appState = nullptr;

	// Main window handle
	HWND ApplicationUI::hMainWnd;

	// UI controls
	HWND hEditSearch;
	HWND hComboFilter;
	HWND hBtnUpdateNow;
	HWND hBtnClearPast;
	HWND hCheckRecurring;
	HWND hComboInterval;
	HWND hComboCategory;
	HWND hAutoUpdateCheck;
	HWND hAutoUpdateInterval;

	void ApplicationUI::Initialize(HWND hWnd, AppState* appState)
	{
		ApplicationUI::appState = appState;
		hMainWnd = hWnd;

		// Initialize common controls
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_BAR_CLASSES;
		InitCommonControlsEx(&icex);

		// Create all UI controls
		CreateMainWindowControls(hWnd);

		// Set up timers
		SetTimer(hWnd, ID_TIMER_UPDATE, appState->GetAutoUpdateIntervalSec() * 1000, NULL);

		// Initial UI update
		EventList::Initialize(&hWnd);
		UpdateUI();
	}

	void ApplicationUI::CreateMainWindowControls(HWND hWnd)
	{
		CreateMainWindowMenus(hWnd);

		// Register custom event list class
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = EventList::EventListWndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = L"CustomEventList";
		if (!RegisterClass(&wc))
		{
			MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		CreateMainButtons(hWnd);
		CreateEventsSearch(hWnd);
		CreateFiltersComboBox(hWnd);
		CreateSortingComboBox(hWnd);
		CreateCategoryComboBox(hWnd);
		CreateAutoUpdateControls(hWnd);
	}

	void ApplicationUI::CreateMainButtons(HWND& hWnd)
	{
		int buttonHorizontalSpacing = 35;
		int buttonVerticalSpacing = 40;
		int currentButton = -1;

		// Create buttons with larger sizes and better spacing
		CreateWindowW(L"BUTTON", L"Створити", WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_ADD, NULL, NULL);

		CreateWindowW(L"BUTTON", L"Редагувати", WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_EDIT, NULL, NULL);

		CreateWindowW(L"BUTTON", L"Видалити", WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_DELETE, NULL, NULL);

		CreateWindowW(L"BUTTON", L"Перемкнути виконання", WS_CHILD | WS_VISIBLE | WS_BORDER,
			10, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_COMPLETE, NULL, NULL);

		currentButton = -1;

		hBtnClearPast = CreateWindowW(L"BUTTON", L"Очистити невиконані",
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			DEFAULT_BUTTON_WIDTH + buttonHorizontalSpacing, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_CLEAR, NULL, NULL);

		hBtnUpdateNow = CreateWindowW(L"BUTTON", L"Оновити список",
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			DEFAULT_BUTTON_WIDTH + buttonHorizontalSpacing, 330 + buttonVerticalSpacing * (++currentButton), DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_UPDATE, NULL, NULL);

		CreateWindowW(L"BUTTON", L"Статистика", WS_CHILD | WS_VISIBLE | WS_BORDER,
			605 - (DEFAULT_BUTTON_WIDTH + 10), 485, DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_METRICS, NULL, NULL);

		CreateWindowW(L"BUTTON", L"Відмінити зміни", WS_CHILD | WS_VISIBLE | WS_BORDER,
			605, 485, DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT, hWnd, (HMENU)ID_BUTTON_DROP_CHANGES, NULL, NULL);
	}

	void ApplicationUI::CreateEventsSearch(HWND& hWnd)
	{
		CreateWindowW(L"STATIC", L"Пошук:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
			500, 330, 60, 25, hWnd, NULL, NULL, NULL);

		hEditSearch = CreateWindowW(L"EDIT", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			570, 330, 200, 25, hWnd, (HMENU)ID_EDIT_SEARCH, NULL, NULL);

		SendMessageW(hEditSearch, EM_LIMITTEXT, MAX_SEARCH_INPUT_LENGTH, 0);
	}

	void ApplicationUI::CreateFiltersComboBox(HWND& hWnd)
	{
		CreateWindowW(L"STATIC", L"Фільтрувати за:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
			440, 370, 120, 25, hWnd, NULL, NULL, NULL);

		hComboFilter = CreateWindowW(L"COMBOBOX", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,
			570, 370, 200, 100, hWnd, (HMENU)ID_COMBO_FILTER, NULL, NULL);

		SendMessageW(hComboFilter, CB_ADDSTRING, 0, (LPARAM)L"Всі");
		SendMessageW(hComboFilter, CB_ADDSTRING, 0, (LPARAM)L"Активні");
		SendMessageW(hComboFilter, CB_ADDSTRING, 0, (LPARAM)L"Завершені");
		SendMessageW(hComboFilter, CB_ADDSTRING, 0, (LPARAM)L"Прострочені");
		SendMessageW(hComboFilter, CB_SETCURSEL, 0, 0);
	}

	void ApplicationUI::CreateSortingComboBox(HWND& hWnd)
	{
		CreateWindowW(L"STATIC", L"Сортувати за:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
			440, 410, 120, 25, hWnd, NULL, NULL, NULL);

		HWND hComboSort = CreateWindowW(L"COMBOBOX", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
			570, 410, 200, 100, hWnd, (HMENU)ID_COMBO_SORT, NULL, NULL);

		SendMessageW(hComboSort, CB_ADDSTRING, 0, (LPARAM)L"Без сортування");
		SendMessageW(hComboSort, CB_ADDSTRING, 0, (LPARAM)L"Назва");
		SendMessageW(hComboSort, CB_ADDSTRING, 0, (LPARAM)L"Пріоритет");
		SendMessageW(hComboSort, CB_ADDSTRING, 0, (LPARAM)L"Дедлайн");
		SendMessageW(hComboSort, CB_ADDSTRING, 0, (LPARAM)L"Дата створення");
		SendMessageW(hComboSort, CB_SETCURSEL, 0, 0);
	}

	void ApplicationUI::CreateCategoryComboBox(HWND& hWnd)
	{
		hComboCategory = CategoryComboBox::Create(
			hWnd,
			570,  // x
			450,  // y
			200,  // width
			30,   // height
			ID_COMBO_CATEGORY
		);
	}

	void ApplicationUI::RefreshCategories()
	{
		CategoryComboBox::UpdateCategories(hComboCategory);
	}

	void ApplicationUI::CreateAutoUpdateControls(HWND hWnd)
	{
		// Label
		CreateWindowW(L"STATIC", L"Авто-оновлення списку",
			WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
			200, 415, 170, 25, hWnd, NULL, NULL, NULL);

		// Checkbox
		hAutoUpdateCheck = CreateWindowW(L"BUTTON", L"Увімкнути",
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			200, 440, 100, 25, hWnd,
			(HMENU)ID_AUTO_UPDATE_CHECK, NULL, NULL);

		// Set initial state from AppState
		Button_SetCheck(hAutoUpdateCheck, appState->IsAutoUpdateEnabled() ? BST_CHECKED : BST_UNCHECKED);

		// Interval label
		CreateWindowW(L"STATIC", L"Інтервал (сек):",
			WS_CHILD | WS_VISIBLE | SS_LEFT,
			200, 470, 120, 25, hWnd, NULL, NULL, NULL);

		// Interval input
		hAutoUpdateInterval = CreateWindowW(L"EDIT", L"",
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
			310, 470, 60, 20, hWnd,
			(HMENU)ID_AUTO_UPDATE_INTERVAL, NULL, NULL);

		SendMessageW(hAutoUpdateInterval, EM_LIMITTEXT, 5, 0); // Max 5 characters


		// Set initial value and enable state
		std::wstring intervalText = std::to_wstring(appState->GetAutoUpdateIntervalSec());
		SetWindowTextW(hAutoUpdateInterval, intervalText.c_str());
		EnableWindow(hAutoUpdateInterval, appState->IsAutoUpdateEnabled());
	}

	void ApplicationUI::CreateMainWindowMenus(HWND hWnd)
	{
		// Create the menu bar
		HMENU hMenuBar = CreateMenu();
		HMENU hFileMenu = CreatePopupMenu();
		HMENU hHelpMenu = CreatePopupMenu();

		// File menu items with keyboard shortcuts
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPENAS, L"Відкрити як...\tCtrl+O");
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, L"Зберегти\tCtrl+S");
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, L"Зберегти як...\tCtrl+Shift+S");
		AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Вийти\tAlt+F4");

		// Help menu items with keyboard shortcut
		AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"Інформація\tF1");

		// Add menus to menu bar
		AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
		AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"Help");

		// Set the menu for the window
		SetMenu(hWnd, hMenuBar);
	}

#pragma region Handle Commands

	void ApplicationUI::HandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		HandleMainCommand(hWnd, wParam, lParam);
		HandleMenusCommand(hWnd, wParam, lParam);
	}

	void ApplicationUI::HandleMainCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		if (LOWORD(wParam) == ID_COMBO_CATEGORY && HIWORD(wParam) == CBN_SELCHANGE)
		{
			HandleCategoryFilter();
			return;
		}

		if (LOWORD(wParam) == ID_AUTO_UPDATE_INTERVAL && HIWORD(wParam) == EN_CHANGE)
		{
			HandleAutoUpdateIntervalInput();
			return;
		}

		switch (LOWORD(wParam))
		{
		case ID_BUTTON_ADD:
			AddNewEvent();
			break;
		case ID_BUTTON_EDIT:
			EditSelectedEvent();
			break;
		case ID_BUTTON_DELETE:
			DeleteSelectedEvent();
			break;
		case ID_BUTTON_COMPLETE:
			ToggleEventCompletion();
			break;
		case ID_BUTTON_UPDATE:
			UpdateEventsStatus();
			break;
		case ID_BUTTON_CLEAR:
			ClearPastEvents();
			break;
		case ID_BUTTON_METRICS:
			ShowMetrics();
			break;

		case ID_BUTTON_DROP_CHANGES:
			appState->DiscardChanges();
			RefilterEvents();
			UpdateUI();
			break;

		case ID_EDIT_SEARCH:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WCHAR searchText[256];
				GetWindowTextW(hEditSearch, searchText, 256);
				appState->SearchEvents(searchText);
				UpdateUI();
			}
			break;

		case ID_COMBO_FILTER:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int selected = SendMessageW(hComboFilter, CB_GETCURSEL, 0, 0);
				appState->FilterEvents(static_cast<AppState::FilterType>(selected));
				UpdateUI();
			}
			break;
		case ID_COMBO_SORT:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				SortEvents();
				UpdateUI();
			}
			break;

		case ID_AUTO_UPDATE_CHECK:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL enabled = Button_GetCheck(hAutoUpdateCheck) == BST_CHECKED;
				appState->SetAutoUpdateEnabled(enabled);
				EnableWindow(hAutoUpdateInterval, enabled);

				// Restart timer with new settings
				KillTimer(hWnd, ID_TIMER_UPDATE);
				if (enabled)
				{
					SetTimer(hWnd, ID_TIMER_UPDATE,
						appState->GetAutoUpdateIntervalSec() * 1000, NULL);
				}
			}
			break;

		case ID_AUTO_UPDATE_INTERVAL:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WCHAR text[32];
				GetWindowTextW(hAutoUpdateInterval, text, 32);
				int interval = _wtoi(text);
				if (interval > 0)
				{
					appState->SetAutoUpdateIntervalSec(interval);

					// Restart timer if auto-update is enabled
					if (appState->IsAutoUpdateEnabled())
					{
						KillTimer(hWnd, ID_TIMER_UPDATE);
						SetTimer(hWnd, ID_TIMER_UPDATE, interval * 1000, NULL);
					}
				}
			}
			break;
		}
	}

	void ApplicationUI::HandleAutoUpdateIntervalInput()
	{
		WCHAR buffer[16];
		GetWindowTextW(hAutoUpdateInterval, buffer, 16);

		int value = _wtoi(buffer);

		if (value > MAX_AUTO_UPDATE_INTERVAL_SEC)
		{
			std::wstring maxIntervalStr = std::to_wstring(MAX_AUTO_UPDATE_INTERVAL_SEC);

			// Clamp to max
			SetWindowTextW(hAutoUpdateInterval, maxIntervalStr.c_str());
		}
	}

	void ApplicationUI::HandleCategoryFilter()
	{
		int selectedIndex = SendMessageW(hComboCategory, CB_GETCURSEL, 0, 0);

		if (selectedIndex == 0)
		{
			// "All Categories" selected
			appState->FilterByCategory(L"");
		}
		else
		{
			// Get selected category text
			WCHAR categoryName[MAX_EVENT_CATEGORY_LENGTH] = L"\0";
			SendMessageW(hComboCategory, CB_GETLBTEXT, selectedIndex, (LPARAM)categoryName);
			appState->FilterByCategory(categoryName);
		}

		UpdateUI();
	}

	void ApplicationUI::HandleMenusCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
			// Handle menu commands
		case IDM_FILE_OPENAS:
		{
			std::wstring filePath = GetOpenFilePath(hWnd);
			if (!filePath.empty())
			{
				appState->LoadEvents(filePath);
				UpdateUI();
			}
			break;
		}
		case IDM_FILE_SAVE:
			if (appState->SaveEvents())
			{
				MessageBoxW(NULL, L"Події успішно збережено у поточний файл!", L"Результат збереження", MB_ICONINFORMATION);
			}

			break;
		case IDM_FILE_SAVEAS:
		{
			std::wstring filePath = GetSaveFilePath(hWnd);
			if (!filePath.empty())
			{
				if (appState->SaveEventsAs(filePath))
				{
					MessageBoxW(NULL, L"Події успішно збережено!", L"Результат збереження", MB_ICONINFORMATION);
				}
			}
			break;
		}
		case IDM_FILE_EXIT:
			PostQuitMessage(0);
			break;
		
		case IDM_HELP_ABOUT:
			MessageBox(hWnd, AboutAppMessage.c_str(), L"Про додаток", MB_OK);
			break;
		}
	}

	void ApplicationUI::HandleTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		if (wParam == ID_TIMER_UPDATE && appState->IsAutoUpdateEnabled())
		{
			appState->UpdateEvents();
			RefilterEvents();
			HandleEventNotifications();
			UpdateUI();
		}
	}

	void ApplicationUI::HandleEventNotifications()
	{
		SYSTEMTIME now;
		GetLocalTime(&now);

		int count = appState->GetAllEventsCount();
		const Event* events = appState->GetAllEvents();

		for (int i = 0; i < count; i++)
		{
			const Event& event = events[i];
			Event& mutableEvent = const_cast<Event&>(event);

			if (event.isPastDeadline 
				&& !event.isCompleted 
				&& event.ShouldNotify(NOTIFICATION_DEADLINE_PASSED))
			{
				ShowNotification(event, L"Дедлайн події вийшов!");
				mutableEvent.MarkNotified(NOTIFICATION_DEADLINE_PASSED);
				continue;
			}

			if (event.isCompleted && !event.isPastDeadline)
			{
				mutableEvent.ResetNotificationStates();
				continue;
			}

			// Check in order of priority
			if (IsDueWithin(event.deadline, 1) &&
				event.ShouldNotify(NOTIFICATION_1H))
			{
				ShowNotification(event, L"Залишилась 1 година до дедлайну події!");
				mutableEvent.MarkNotified(NOTIFICATION_1H);
				continue;
			}

			if (IsDueWithin(event.deadline, 12) &&
				event.ShouldNotify(NOTIFICATION_12H))
			{
				ShowNotification(event, L"Залишилось 12 годин до дедлайну події!");
				mutableEvent.MarkNotified(NOTIFICATION_12H);
				continue; 
			}

			if (IsDueWithin(event.deadline, 24) &&
				event.ShouldNotify(NOTIFICATION_24H))
			{
				ShowNotification(event, L"Залишилось 24 години до дедлайну події!");
				mutableEvent.MarkNotified(NOTIFICATION_24H);
			}
		}
	}

	void ApplicationUI::ShowNotification(const Event& event, const std::wstring& message)
	{
		NOTIFYICONDATA nid = { sizeof(nid) };
		nid.hWnd = hMainWnd;
		nid.uFlags = NIF_INFO;
		nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
		wcscpy_s(nid.szInfoTitle, (L"Нагадування про подію - " + event.name).c_str());
		wcscpy_s(nid.szInfo, message.c_str());

		nid.hIcon = LoadIcon(NULL, IDI_WARNING);

		Shell_NotifyIcon(NIM_ADD, &nid);
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

#pragma endregion


	void ApplicationUI::UpdateUI()
	{
		appState->HasFilters()
			? EventList::SetEventsList(appState->GetFilteredEvents(), appState->GetFilteredEventsCount())
			: EventList::SetEventsList(appState->GetAllEvents(), appState->GetAllEventsCount());

		EventList::Update();
	}

	void ApplicationUI::AddNewEvent()
	{
		Event newEvent;
		if (EventDialog::Show(hMainWnd, &newEvent))
		{
			if (appState->ContainsSameEvent(&newEvent))
			{
				MessageBoxW(
					nullptr,
					L"Не вдалося добавити нову подію, оскільки подія з таким іменем та дедлайном вже існує!",
					L"Помилка створення події",
					MB_ICONERROR | MB_OK
				);
				return;
			}

			appState->AddEvent(&newEvent);
			RefilterEvents();
			SendDlgItemMessage(hMainWnd, ID_COMBO_SORT, CB_SETCURSEL, 0, 0);
			UpdateUI();
		}
	}

	void ApplicationUI::EditSelectedEvent()
	{
		Event* originalEvent = nullptr;
		Event* eventToModify = nullptr;

		if (!EventList::TryGetSelectedEventFromList(originalEvent))
		{
			MessageBoxW(
				nullptr,
				L"Для редагування події, необхідно обрати її в списку!",
				L"Помилка редагування",
				MB_ICONERROR | MB_OK
			);

			return;
		}

		eventToModify = appState->FindEventByGUID(originalEvent->uniqueID);

		if (eventToModify == nullptr)
		{
			MessageBoxW(
				nullptr,
				L"Обраної події немає в основному списку, можливо дані були модифіковані, оновіть список.",
				L"Помилка редагування",
				MB_ICONERROR | MB_OK
			);

			return;
		}

		Event initialEvent = *eventToModify;

		if (EventDialog::Show(hMainWnd, eventToModify, originalEvent))
		{
			if (appState->ContainsSameEvent(eventToModify))
			{
				*eventToModify = initialEvent;
				MessageBoxW(
					nullptr,
					L"Не вдалося відредагувати подію, оскільки подія з таким іменем та дедлайном вже існує!",
					L"Помилка редагування події",
					MB_ICONERROR | MB_OK
				);
				return;
			}

			appState->UpdateEvents();
			RefilterEvents();
			UpdateUI();
		}
	}

	void ApplicationUI::RefilterEvents()
	{
		if (appState->HasFilters())
		{
			appState->ApplyCurrentFilter();
		}
	}

	void ApplicationUI::DeleteSelectedEvent()
	{
		Event* selectedEvent = nullptr;

		if (!EventList::TryGetSelectedEventFromList(selectedEvent))
		{
			MessageBoxW(
				nullptr,
				L"Для видалення події, необхідно обрати її в списку!",
				L"Помилка видалення",
				MB_ICONERROR | MB_OK
			);

			return;
		}

		if (appState->DeleteEvent(selectedEvent))
		{
			RefilterEvents();
			UpdateUI();
		}
		else
		{
			MessageBoxW(
				nullptr,
				L"Не вдалося видалити подію. Спробуйте ще раз.",
				L"Помилка видалення",
				MB_ICONERROR | MB_OK
			);
		}
	}

	void ApplicationUI::ToggleEventCompletion()
	{
		Event* selectedEvent = nullptr;

		if (!EventList::TryGetSelectedEventFromList(selectedEvent))
		{
			MessageBoxW(
				nullptr,
				L"Для зміни статусу події, необхідно обрати її в списку!",
				L"Помилка",
				MB_ICONERROR | MB_OK
			);

			return;
		}

		if (appState->ToggleEventCompletion(selectedEvent))
		{
			RefilterEvents();
			UpdateUI();
		}
		else
		{
			MessageBoxW(
				nullptr,
				L"Не вдалося змінити статус завершення події. Можливо дедлайн був прострочений.",
				L"Помилка",
				MB_ICONERROR | MB_OK
			);
		}
	}

	void ApplicationUI::UpdateEventsStatus()
	{
		appState->UpdateEvents();
		RefilterEvents();
		UpdateUI();
	}

	void ApplicationUI::ClearPastEvents()
	{
		appState->ClearPastEvents();
		RefilterEvents();
		UpdateUI();
	}

	void ApplicationUI::ShowMetrics()
	{
		int total = appState->GetAllEventsCount();
		int completed = 0;
		int pending = 0;
		int pastDue = 0;

		for (int i = 0; i < total; ++i)
		{
			Event* targetEvent = appState->GetEvent(i);
			if (targetEvent->isCompleted)
			{
				completed++;
			}
			else if (targetEvent->isPastDeadline)
			{
				pastDue++;
			}
			else
			{
				pending++;
			}
		}

		wchar_t buffer[512];
		swprintf(buffer, 512,
			L"Статистика подій :\n"
			L"Загальна кількість подій : %d\n"
			L"Виконані : %d (%.1f%%)\n"
			L"Виконуються : %d (%.1f%%)\n"
			L"Прострочені : %d (%.1f%%)",
			total,
			completed, total > 0 ? (completed * 100.0f / total) : 0.0f,
			pending, total > 0 ? (pending * 100.0f / total) : 0.0f,
			pastDue, total > 0 ? (pastDue * 100.0f / total) : 0.0f
		);

		MessageBoxW(hMainWnd, buffer, L"Метрики подій", MB_OK | MB_ICONINFORMATION);
	}

	void ApplicationUI::SortEvents()
	{
		int sortType = SendDlgItemMessage(hMainWnd, ID_COMBO_SORT, CB_GETCURSEL, 0, 0) - 1;
		appState->SortEvents((AppState::SortType)sortType);
		EventList::Update();
	}
}
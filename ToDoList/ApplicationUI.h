#pragma once
#include <windows.h>
#include <commctrl.h>
#include "AppState.h"
#include "Event.h"

namespace ApplicationUserInterface
{
	class ApplicationUI
	{
	public:
		static HWND hMainWnd;

		static void Initialize(HWND hWnd, AppState* appState);
		static void HandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
		static void HandleTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
		static void UpdateUI();
		static void RefreshCategories();

	private:
		static void HandleCategoryFilter();
		static void HandleMenusCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
		static void HandleMainCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
		static void HandleAutoUpdateIntervalInput();
		static void HandleEventNotifications();

		static void CreateMainWindowControls(HWND hWnd);
		static void CreateAutoUpdateControls(HWND hWnd);
		static void CreateMainButtons(HWND& hWnd);
		static void CreateEventsSearch(HWND& hWnd);
		static void CreateFiltersComboBox(HWND& hWnd);
		static void CreateSortingComboBox(HWND& hWnd);
		static void CreateCategoryComboBox(HWND& hWnd);
		static void CreateMainWindowMenus(HWND hWnd);

		static void AddNewEvent();
		static void EditSelectedEvent();
		static void DeleteSelectedEvent();
		static void ToggleEventCompletion();
		static void UpdateEventsStatus();
		static void ClearPastEvents();
		static void ShowMetrics();
		static void RefilterEvents();
		static void SortEvents();

		static void ShowNotification(const Event& event, const std::wstring& message);

		static AppState* appState;
	};
}
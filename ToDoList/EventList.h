#pragma once

#include "Windows.h"
#include <windowsx.h>
#include "Utilities.h"
#include "AppState.h"

class EventList
{
public:
	// Custom drawing constants
	static const int EVENT_ITEM_HEIGHT = 90;
	static const int EVENT_ITEM_MARGIN = 5;
	static const int EVENT_ITEM_PADDING = 10;


	static void Initialize(HWND* parentWnd);

	// Custom window procedure for our event list
	static LRESULT CALLBACK EventListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static void DrawEventItem(HDC hdc, const RECT* rcItem, const Event* event, BOOL isSelected);
	static void DrawEventBackground(HDC hdc, const RECT* rcItem, const Event* event, BOOL isSelected);
	static COLORREF GetEventBackgroundColor(const Event* event, BOOL isSelected);
	static COLORREF GetEventTextColor(const Event* event, BOOL isSelected);
	static void DrawEventHeader(HDC hdc, RECT* rcText, const Event* event, BOOL isSelected);
	static void DrawEventDescription(HDC hdc, RECT* rcText, const Event* event);
	static void DrawEventStatusAndCategory(HDC hdc, RECT* rcText, const Event* event);


	static void SetEventsList(Event* eventsList, int eventsCount);
	static void HandleEventListItemClick(LPARAM& lParam, HWND hWnd);
	static void PaintEventListView(HWND hWnd);
	static bool TryGetSelectedEventFromList(Event*& targetEvent);
	static int CurrentSelectedEventIndex();

	static void Update();
	static void Redraw();


private:
	static HWND parentWnd;
	static HWND hEventList;
	static Event* eventsArray;
	static int eventsCount;
	static int selectedEventIndex;
};
#define UNICODE
#define _UNICODE

#include "EventList.h"
#include "resource.h"
#include "EventDetailsDialog.h"

HWND EventList::parentWnd = nullptr;
HWND EventList::hEventList = nullptr;
Event* EventList::eventsArray = nullptr;
int EventList::eventsCount = 0;
int EventList::selectedEventIndex = -1;

void EventList::Initialize(HWND* parentWnd)
{
	EventList::selectedEventIndex = -1;
	EventList::parentWnd = *parentWnd;

	// Create custom event list with proper parameters
	hEventList = CreateWindowExW(
		WS_EX_CLIENTEDGE,                // Extended style
		L"CustomEventList",               // Class name
		L"",                              // Window text
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | WS_CLIPCHILDREN,
		10, 10,                          // Position
		760, 300,                        // Size
		*parentWnd,                        // Parent window
		(HMENU)ID_EVENT_LIST,             // ID
		GetModuleHandle(NULL),            // Instance
		NULL);                           // Additional data

	// Initial update
	Update();
}


LRESULT CALLBACK EventList::EventListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PaintEventListView(hWnd);
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		HandleEventListItemClick(lParam, hWnd);
		return 0;
	}

	case WM_MOUSEWHEEL:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO) };
		si.fMask = SIF_PAGE | SIF_RANGE;
		GetScrollInfo(hWnd, SB_VERT, &si);

		if (si.nMax > 0)
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			int scrollLines = 1; // Number of lines to scroll per wheel notch

			int currentPos = GetScrollPos(hWnd, SB_VERT);
			int newPos = currentPos - (zDelta / WHEEL_DELTA) * scrollLines;

			// Clamp to valid range
			newPos = max(0, min(newPos, si.nMax - (int)si.nPage + 1));

			if (newPos != currentPos)
			{
				SetScrollPos(hWnd, SB_VERT, newPos, TRUE);
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		return 0;
	}


	case WM_VSCROLL:
	{
		SCROLLINFO si = { sizeof(SCROLLINFO) };
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_VERT, &si);

		int oldPos = si.nPos;
		int itemHeight = EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN;

		switch (LOWORD(wParam))
		{
		case SB_LINEUP:        si.nPos -= 1; break;   // One item up
		case SB_LINEDOWN:      si.nPos += 1; break;   // One item down
		case SB_PAGEUP:        si.nPos -= si.nPage; break;
		case SB_PAGEDOWN:      si.nPos += si.nPage; break;
		case SB_THUMBTRACK:    si.nPos = si.nTrackPos; break;
		case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
		case SB_TOP:           si.nPos = si.nMin; break;
		case SB_BOTTOM:        si.nPos = si.nMax; break;
		default:               break;
		}

		// Clamp the position
		si.nPos = max(0, min(si.nPos, si.nMax - (int)si.nPage + 1));

		if (si.nPos != oldPos)
		{
			SetScrollPos(hWnd, SB_VERT, si.nPos, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		return 0;
	}

	case WM_SIZE:
	{
		// Update scrollbar info
		SCROLLINFO si = { sizeof(SCROLLINFO) };
		si.fMask = SIF_PAGE | SIF_RANGE;
		si.nMin = 0;
		si.nMax = eventsCount;

		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		si.nPage = rcClient.bottom / (EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN);

		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		InvalidateRect(hWnd, NULL, TRUE);
		return 0;
	}

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

void EventList::SetEventsList(Event* eventsList, int eventsCount)
{
	EventList::eventsArray = eventsList;
	EventList::eventsCount = eventsCount;
}

void EventList::HandleEventListItemClick(LPARAM& lParam, HWND hWnd)
{
	int yPos = GET_Y_LPARAM(lParam);
	int scrollPos = GetScrollPos(hWnd, SB_VERT);

	// Calculate which item was clicked (-1 means background)
	int itemsPerPage = GetClientHeight(hWnd) / (EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN);
	int itemIndex = -1;

	if (yPos < itemsPerPage * (EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN))
	{
		itemIndex = scrollPos + yPos / (EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN);
		if (itemIndex >= eventsCount)
		{
			itemIndex = -1;  // Clicked below last item
		}
	}

	if (itemIndex >= 0)  // Clicked on an item
	{
		if (itemIndex == selectedEventIndex)
		{
			// Clicked already selected item - show details
			Event* selectedEvent = &eventsArray[itemIndex];
			if (selectedEvent)
			{
				EventDetailsDialog::Show(hWnd, selectedEvent);
			}
		}
		else
		{
			// Select new item
			selectedEventIndex = itemIndex;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
	else  // Clicked on background
	{
		// Clear selection if something was selected
		if (selectedEventIndex != -1)
		{
			selectedEventIndex = -1;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
}

void EventList::PaintEventListView(HWND hWnd)
{
	// Start painting
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	// Get client area
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	// Create memory DC for double buffering
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

	// Fill background
	FillRect(hdcMem, &rcClient, (HBRUSH)(COLOR_WINDOW + 1));

	if (eventsArray == nullptr || eventsCount <= 0)
	{
		EndPaint(hWnd, &ps);
		return;
	}

	// Get scroll position
	SCROLLINFO si = { sizeof(SCROLLINFO) };
	si.fMask = SIF_POS;
	GetScrollInfo(hWnd, SB_VERT, &si);
	int scrollPos = si.nPos;

	// Calculate how many items fit in the client area
	const int itemHeightWithMargin = EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN;
	int itemsPerPage = rcClient.bottom / itemHeightWithMargin;

	// Calculate first and last visible items
	int firstVisible = scrollPos;
	int lastVisible = min(firstVisible + itemsPerPage, eventsCount - 1);

	// Check if the visible range is valid
	if (firstVisible < 0 || lastVisible < 0 || firstVisible >= eventsCount || lastVisible >= eventsCount)
	{
		EndPaint(hWnd, &ps);
		return;
	}

	// Draw visible items
	for (int i = firstVisible; i <= lastVisible; i++)
	{
		RECT rcItem = {
			0,
			(i - firstVisible) * itemHeightWithMargin,
			rcClient.right,
			(i - firstVisible) * itemHeightWithMargin + EVENT_ITEM_HEIGHT
		};

		BOOL isSelected = (i == selectedEventIndex);
		DrawEventItem(hdcMem, &rcItem, &eventsArray[i], isSelected);
	}

	// Copy to screen
	BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);

	// Clean up
	SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);

	EndPaint(hWnd, &ps);
}


void EventList::Update()
{
	if (!hEventList) return;

	RECT rcClient;
	GetClientRect(hEventList, &rcClient);

	// Calculate how many items fit in the client area
	const int itemHeightWithMargin = EVENT_ITEM_HEIGHT + EVENT_ITEM_MARGIN;
	int itemsPerPage = rcClient.bottom / itemHeightWithMargin;

	// Get current scroll position
	SCROLLINFO siCurrent = { sizeof(SCROLLINFO) };
	siCurrent.fMask = SIF_POS;
	GetScrollInfo(hEventList, SB_VERT, &siCurrent);
	int currentScrollPos = siCurrent.nPos;

	// Update scrollbar information
	SCROLLINFO si = { sizeof(SCROLLINFO) };
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = max(0, eventsCount - 1);  // Ensure non-negative
	si.nPage = itemsPerPage;

	// Adjust scroll position if needed (when deleting items)
	if (eventsCount <= itemsPerPage)
	{
		// Hide scrollbar when all items fit
		ShowScrollBar(hEventList, SB_VERT, FALSE);
		currentScrollPos = 0; // Reset to top
	}
	else
	{
		// Show and configure scrollbar when needed
		ShowScrollBar(hEventList, SB_VERT, TRUE);

		// Ensure scroll position stays within valid range
		if (currentScrollPos > eventsCount - itemsPerPage)
		{
			currentScrollPos = max(0, eventsCount - itemsPerPage);
		}
	}

	// Set the adjusted scroll position
	si.nPos = currentScrollPos;
	SetScrollInfo(hEventList, SB_VERT, &si, TRUE);

	// Update the actual scroll position
	SetScrollPos(hEventList, SB_VERT, currentScrollPos, TRUE);

	// Force redraw
	InvalidateRect(hEventList, NULL, TRUE);
	UpdateWindow(hEventList);
}

void EventList::DrawEventItem(HDC hdc, const RECT* rcItem, const Event* event, BOOL isSelected)
{
	int savedDC = SaveDC(hdc);

	// Draw background and border
	DrawEventBackground(hdc, rcItem, event, isSelected);

	// Calculate text areas
	RECT rcText = *rcItem;
	rcText.left += EVENT_ITEM_PADDING + EVENT_ITEM_MARGIN;
	rcText.top += EVENT_ITEM_PADDING + EVENT_ITEM_MARGIN;
	rcText.right -= EVENT_ITEM_PADDING + EVENT_ITEM_MARGIN;
	rcText.bottom -= EVENT_ITEM_PADDING + EVENT_ITEM_MARGIN;

	// Draw header (name + deadline)
	DrawEventHeader(hdc, &rcText, event, isSelected);

	DrawEventStatusAndCategory(hdc, &rcText, event);

	DrawEventDescription(hdc, &rcText, event);


	RestoreDC(hdc, savedDC);
}

void EventList::DrawEventBackground(HDC hdc, const RECT* rcItem, const Event* event, BOOL isSelected)
{
	COLORREF bgColor = GetEventBackgroundColor(event, isSelected);
	COLORREF borderColor = isSelected ? RGB(0, 0, 128) : RGB(200, 200, 200);

	HBRUSH hBrush = CreateSolidBrush(bgColor);
	HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);

	HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
	HGDIOBJ hOldPen = SelectObject(hdc, hPen);

	RoundRect(hdc,
		rcItem->left + EVENT_ITEM_MARGIN,
		rcItem->top + EVENT_ITEM_MARGIN,
		rcItem->right - EVENT_ITEM_MARGIN,
		rcItem->bottom - EVENT_ITEM_MARGIN,
		8, 8);

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}

COLORREF EventList::GetEventBackgroundColor(const Event* event, BOOL isSelected)
{
	if (isSelected)
	{
		return GetSysColor(COLOR_HIGHLIGHT);
	}

	if (event->isCompleted)
	{
		return EVENT_DONE_COLOR;
	}

	if (event->isPastDeadline)
	{
		return EVENT_PASTDUE_COLOR;
	}

	switch (event->priority)
	{
	case EventPriority::Critical: return EVENT_PRIORITY_CRITICAL_COLOR;
	case EventPriority::High:    return EVENT_PRIORITY_HIGH_COLOR;
	case EventPriority::Medium:  return EVENT_PRIORITY_MEDIUM_COLOR;
	case EventPriority::Low:     return EVENT_PRIORITY_LOW_COLOR;
	default:                     return GetSysColor(COLOR_WINDOW);
	}
}

COLORREF EventList::GetEventTextColor(const Event* event, BOOL isSelected)
{
	if (isSelected)
	{
		return GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	if (event->isPastDeadline)
	{
		return RGB(200, 0, 0);    // Red for past due
	}
	if (event->isCompleted)
	{
		return RGB(0, 128, 0);    // Green for completed
	}
	return GetSysColor(COLOR_WINDOWTEXT);
}

void EventList::DrawEventHeader(HDC hdc, RECT* rcText, const Event* event, BOOL isSelected)
{
	// Set up text color
	SetTextColor(hdc, GetEventTextColor(event, isSelected));
	SetBkMode(hdc, TRANSPARENT);

	// Create and select font
	HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

	// Calculate header area
	RECT rcHeader = *rcText;
	rcHeader.bottom = rcHeader.top + 24;

	// First measure the deadline text to reserve space
	std::wstring deadlineStr = L"Дедлайн: " + SystemTimeToString(event->deadline);
	RECT rcDeadline = rcHeader;
	DrawTextW(hdc, deadlineStr.c_str(), -1, &rcDeadline, DT_CALCRECT | DT_RIGHT | DT_END_ELLIPSIS);
	int deadlineWidth = rcDeadline.right - rcDeadline.left + 15; // Add some padding

	// Create rectangle for name with limited width
	RECT rcName = rcHeader;
	rcName.right = rcHeader.right - deadlineWidth;

	// Draw event name (left-aligned with ellipsis)
	std::wstring nameText = L"Назва: " + event->name;
	DrawTextW(hdc, nameText.c_str(), -1, &rcName, DT_LEFT | DT_END_ELLIPSIS);

	// Draw deadline (right-aligned at original position)
	DrawTextW(hdc, deadlineStr.c_str(), -1, &rcHeader, DT_RIGHT | DT_END_ELLIPSIS);

	// Clean up font
	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

void EventList::DrawEventDescription(HDC hdc, RECT* rcText, const Event* event)
{
	HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

	RECT rcDesc = *rcText;
	rcDesc.top = rcText->top + 38;  // Below header
	rcDesc.bottom = rcDesc.top + 50;
	std::wstring descriptionText = L"Опис : " + event->description;

	DrawTextW(hdc, descriptionText.c_str(), -1, &rcDesc, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

void EventList::DrawEventStatusAndCategory(HDC hdc, RECT* rcText, const Event* event)
{
	HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

	RECT rcFooter = *rcText;
	rcFooter.top = rcText->top + 20;  // Below description
	rcFooter.bottom = rcFooter.top + 50;

	// Create status string with icon-like indicators
	std::wstring statusValue;
	std::wstring statusText = L"Статус : ";
	if (event->isCompleted)
	{
		statusValue = L"✓ " + event->GetStatusString();
	}
	else if (event->isPastDeadline)
	{
		statusValue = L"⚠ " + event->GetStatusString();
	}
	else
	{
		statusValue = L"▶ " + event->GetStatusString();
	}

	statusText += statusValue;

	// Draw status (left-aligned)
	DrawTextW(hdc, statusText.c_str(), -1, &rcFooter, DT_LEFT | DT_END_ELLIPSIS);

	// Draw category (right-aligned)
	DrawTextW(hdc, event->category.c_str(), -1, &rcFooter, DT_RIGHT | DT_END_ELLIPSIS);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

bool EventList::TryGetSelectedEventFromList(Event*& targetEvent)
{
	if (selectedEventIndex >= 0 && selectedEventIndex < eventsCount)
	{
		targetEvent = &eventsArray[selectedEventIndex];
		return true;
	}
	return false;
}

int EventList::CurrentSelectedEventIndex()
{
	return selectedEventIndex;
}

void EventList::Redraw()
{
	PaintEventListView(parentWnd);
}

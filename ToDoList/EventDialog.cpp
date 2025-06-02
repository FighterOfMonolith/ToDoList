#define UNICODE
#define _UNICODE

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"
#include "appState.h"
#include "EventDialog.h"
#include "Utilities.h"
#include "ApplicationUI.h"
#pragma comment(lib, "comctl32.lib")

// Common

extern AppState* appState;
bool isDateValid = true;
int inputDialogUpdateIntervalSec = 3;
bool editingEvent = false;

// Styles

static HBRUSH hBrushWarning = NULL;
static HBRUSH hBrushNormal = NULL;
static HFONT hBoldFont = NULL;
static HFONT hDialogFont = NULL;

bool EventDialog::Show(HWND hParent, Event* event, Event* originalEvent)
{
	editingEvent = originalEvent != nullptr;

	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_EVENT_DIALOG),
		hParent,
		DialogProc,
		reinterpret_cast<LPARAM>(event)
	) == IDOK;
}

INT_PTR CALLBACK EventDialog::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Event* event = NULL;

	switch (msg)
	{
	case WM_INITDIALOG:
		event = reinterpret_cast<Event*>(lParam);
		OnInitDialog(hDlg, event);
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_STATIC_DATE_WARNING))
		{
			HDC hdc = (HDC)wParam;

			// If you have a bold font, select it
			if (hBoldFont)
			{
				SelectObject(hdc, hBoldFont);
			}
			else
			{
				// Reset to default font (if needed)
				SelectObject(hdc, GetStockObject(SYSTEM_FONT));
			}
			if (!isDateValid)
				SetTextColor(hdc, RGB(255, 0, 0)); // Red for warnings
			else
				SetTextColor(hdc, RGB(0, 128, 0)); // Green for valid
			SetBkMode(hdc, TRANSPARENT);
			return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
		}
		break;


	case WM_NOTIFY:
	{
		LPNMHDR pnmh = (LPNMHDR)lParam;
		if (pnmh->code == DTN_DATETIMECHANGE &&
			(pnmh->idFrom == IDC_DATEPICKER || pnmh->idFrom == IDC_TIMEPICKER))
		{
			UpdateDateTimeWarning(hDlg);
			return TRUE;
		}
		break;
	}

	case WM_TIMER:
		if (wParam == ID_UPDATE_TIMER)
		{
			UpdateDateTimeWarning(hDlg);
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK_RECURRING:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL isChecked = IsDlgButtonChecked(hDlg, IDC_CHECK_RECURRING);
				EnableWindow(GetDlgItem(hDlg, IDC_COMBO_INTERVAL), isChecked);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CUSTOM_DAYS),
					isChecked &&
					GetSelectedReccurenceInterval(hDlg) == RecurrenceInterval::Custom);
			}
			return TRUE;

		case IDC_COMBO_INTERVAL:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CUSTOM_DAYS),
					GetSelectedReccurenceInterval(hDlg) == RecurrenceInterval::Custom);
			}
			return TRUE;

		case IDC_ADD_CATEGORY_BTN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				AddNewCategory(hDlg);
			}
			return TRUE;

		case IDC_EDIT_NAME:
		case IDC_EDIT_DESC:
		case IDC_EDIT_NOTES:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				UpdateCharCount(hDlg, LOWORD(wParam));
			}
			return TRUE;

		case ID_CREATE_EVENT_BTN:
			if (ValidateFields(hDlg))
			{
				OnOK(hDlg, event);
				EndDialog(hDlg, IDOK);
			}
			return TRUE;

		case IDCANCEL:
		case ID_CANCEL_EVENT_CREATION_BTN:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		ClearResources(hDlg);
		break;
	}
	return FALSE;
}

void EventDialog::ClearResources(HWND hDlg)
{
	KillTimer(hDlg, ID_UPDATE_TIMER);
	// Clean up brushes
	if (hBrushWarning) DeleteObject(hBrushWarning);
	if (hBrushNormal) DeleteObject(hBrushNormal);
	if (hBoldFont)
	{
		DeleteObject(hBoldFont);
		hBoldFont = NULL;
	}

	if (hDialogFont)
	{
		DeleteObject(hDialogFont);
		hDialogFont = NULL;
	}
}

void EventDialog::OnInitDialog(HWND hDlg, Event* event)
{
	ConfigureStaticLabelsStyle(hDlg);

	// Set text fields
	SetDlgItemText(hDlg, IDC_EDIT_NAME, event->name.c_str());
	SetDlgItemText(hDlg, IDC_EDIT_DESC, event->description.c_str());
	SetDlgItemText(hDlg, IDC_EDIT_NOTES, event->notes.c_str());

	// Set text limits
	SendDlgItemMessage(hDlg, IDC_EDIT_NAME, EM_SETLIMITTEXT, MAX_EVENT_NAME_LENGTH, 0);
	SendDlgItemMessage(hDlg, IDC_EDIT_DESC, EM_SETLIMITTEXT, MAX_EVENT_DESC_LENGTH, 0);
	SendDlgItemMessage(hDlg, IDC_EDIT_NOTES, EM_SETLIMITTEXT, MAX_EVENT_NOTES_LENGTH, 0);

	HWND hDate = GetDlgItem(hDlg, IDC_DATEPICKER);
	HWND hTime = GetDlgItem(hDlg, IDC_TIMEPICKER);

	// Initialize date picker
	DateTime_SetFormat(hDate, L"dd/MM/yyyy");
	DateTime_SetSystemtime(hDate, GDT_VALID, &event->deadline);

	// Initialize time picker 
	SYSTEMTIME stTime = event->deadline;
	stTime.wSecond = 0;
	stTime.wMilliseconds = 0;

	DateTime_SetFormat(hTime, L"HH:mm");
	DateTime_SetSystemtime(hTime, GDT_VALID, &stTime);

	// Populate the category combo box
	HWND hCategory = GetDlgItem(hDlg, IDC_COMBO_CATEGORY);
	for (size_t i = 0; i < AppConfig::GetCategoryCount(); i++)
	{
		SendMessage(hCategory, CB_ADDSTRING, 0, (LPARAM)AppConfig::GetCategory(i));
	}

	// Set the current category in the combo box
	int index = SendMessage(hCategory, CB_FINDSTRINGEXACT, -1, (LPARAM)event->category.c_str());
	if (index != CB_ERR)
		SendMessage(hCategory, CB_SETCURSEL, index, 0);
	else if (AppConfig::GetCategoryCount() > 0)
		SendMessage(hCategory, CB_SETCURSEL, 0, 0);

	// Populate the priority combo box
	HWND hPriority = GetDlgItem(hDlg, IDC_COMBO_PRIORITY);
	SendMessage(hPriority, CB_ADDSTRING, 0, (LPARAM)L"Низький");
	SendMessage(hPriority, CB_ADDSTRING, 0, (LPARAM)L"Середній");
	SendMessage(hPriority, CB_ADDSTRING, 0, (LPARAM)L"Високий");
	SendMessage(hPriority, CB_ADDSTRING, 0, (LPARAM)L"Критичний");
	SendMessage(hPriority, CB_SETCURSEL, (WPARAM)event->priority, 0);

	// Set recurring and completed checkbox states
	CheckDlgButton(hDlg, IDC_CHECK_RECURRING, event->isRecurring ? BST_CHECKED : BST_UNCHECKED);

	// Populate recurrence interval combo box
	HWND hInterval = GetDlgItem(hDlg, IDC_COMBO_INTERVAL);
	SendMessage(hInterval, CB_ADDSTRING, 0, (LPARAM)L"Щоденна");
	SendMessage(hInterval, CB_ADDSTRING, 0, (LPARAM)L"Щотижнева");
	SendMessage(hInterval, CB_ADDSTRING, 0, (LPARAM)L"Щомісячна");
	SendMessage(hInterval, CB_ADDSTRING, 0, (LPARAM)L"Щорічна");
	SendMessage(hInterval, CB_ADDSTRING, 0, (LPARAM)L"Власний інтервал");

	if (event->recurrenceInterval != RecurrenceInterval::None)
		SendMessage(hInterval, CB_SETCURSEL, (WPARAM)((int)event->recurrenceInterval - 1), 0);
	else
		SendMessage(hInterval, CB_SETCURSEL, 0, 0);


	// Enable/disable recurrence interval and custom days input based on recurrence checkbox
	EnableWindow(hInterval, event->isRecurring);

	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CUSTOM_DAYS),
		event->isRecurring &&
		GetSelectedReccurenceInterval(hDlg) == RecurrenceInterval::Custom);
	// Set custom recurrence days
	WCHAR buffer[32];
	swprintf(buffer, 32, L"%d", event->customRecurrenceDays);
	SetDlgItemText(hDlg, IDC_EDIT_CUSTOM_DAYS, buffer);

	// Set the warning text color
	HWND hWarning = GetDlgItem(hDlg, IDC_STATIC_DATE_WARNING);
	HDC hdc = GetDC(hWarning);
	SetTextColor(hdc, RGB(0, 0, 0)); // Default black
	ReleaseDC(hWarning, hdc);

	// Update character count for name, description, and notes fields
	UpdateCharCount(hDlg, IDC_EDIT_NAME);
	UpdateCharCount(hDlg, IDC_EDIT_DESC);
	UpdateCharCount(hDlg, IDC_EDIT_NOTES);

	hBoldFont = CreateFont(0, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"MS Shell Dlg");

	hBrushWarning = CreateSolidBrush(RGB(255, 255, 255));
	hBrushNormal = CreateSolidBrush(RGB(255, 255, 255));

	SetTimer(hDlg, ID_UPDATE_TIMER, inputDialogUpdateIntervalSec * 1000, NULL);
	UpdateDateTimeWarning(hDlg);

	if (editingEvent)
	{
		SetDlgItemText(hDlg, ID_CREATE_EVENT_BTN, L"Змінити");
	}
}

void EventDialog::ConfigureStaticLabelsStyle(HWND hDlg)
{
	hDialogFont = CreateFont(
		16, 0, 0, 0,         // Height, Width (0=default), Escapement, Orientation
		FW_BOLD,              // Bold weight
		FALSE, FALSE, FALSE,  // Italic, Underline, Strikeout
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"MS Shell Dlg"       // Same font family as dialog
	);

	SendDlgItemMessage(hDlg, IDC_EVENT_NAME_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_DESC_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_NOTES_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_DATE_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_INTERVAL_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_CATEGORY_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_TIME_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_PRIORITY_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_EVENT_DAYS_LABEL, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_STATIC_DATE_NOTE, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, IDC_CHECK_RECURRING, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, ID_CREATE_EVENT_BTN, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
	SendDlgItemMessage(hDlg, ID_CANCEL_EVENT_CREATION_BTN, WM_SETFONT, (WPARAM)hDialogFont, TRUE);
}

void EventDialog::UpdateDateTimeWarning(HWND hDlg)
{
	SYSTEMTIME stDate, stTime, stNow;
	GetLocalTime(&stNow);

	// Get current values from pickers
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATEPICKER), &stDate);
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_TIMEPICKER), &stTime);

	// Combine date and time
	stDate.wHour = stTime.wHour;
	stDate.wMinute = stTime.wMinute;
	stDate.wSecond = 0;
	stDate.wMilliseconds = 0;

	// Compare with current time
	isDateValid = !IsSystemTimeInPast(stDate);

	HWND hWarning = GetDlgItem(hDlg, IDC_STATIC_DATE_WARNING);

	// Set new text
	SetWindowText(hWarning,
		isDateValid ? L"✓ Дата/час коректні"
		: L"⚠ Увага: Обрані дата/час вже минули!");

	// Force complete redraw
	RedrawWindow(hWarning, NULL, NULL,
		RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}



void EventDialog::OnOK(HWND hDlg, Event* event)
{
	WCHAR buffer[1025];

	GetDlgItemText(hDlg, IDC_EDIT_NAME, buffer, MAX_EVENT_NAME_LENGTH + 1);
	event->name = buffer;

	GetDlgItemText(hDlg, IDC_EDIT_DESC, buffer, MAX_EVENT_DESC_LENGTH + 1);
	event->description = buffer;

	GetDlgItemText(hDlg, IDC_EDIT_NOTES, buffer, MAX_EVENT_NOTES_LENGTH + 1);
	event->notes = buffer;

	int sel = SendDlgItemMessage(hDlg, IDC_COMBO_CATEGORY, CB_GETCURSEL, 0, 0);
	if (sel != CB_ERR)
	{
		SendDlgItemMessage(hDlg, IDC_COMBO_CATEGORY, CB_GETLBTEXT, sel, (LPARAM)buffer);
		event->category = buffer;
	}

	SYSTEMTIME stDate, stTime;
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATEPICKER), &stDate);
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_TIMEPICKER), &stTime);

	event->deadline.wYear = stDate.wYear;
	event->deadline.wMonth = stDate.wMonth;
	event->deadline.wDay = stDate.wDay;
	event->deadline.wHour = stTime.wHour;
	event->deadline.wMinute = stTime.wMinute;
	event->deadline.wMilliseconds = 0;
	event->deadline.wDayOfWeek = 0;
	event->deadline.wSecond = 0;

	event->priority = (EventPriority)SendDlgItemMessage(hDlg, IDC_COMBO_PRIORITY, CB_GETCURSEL, 0, 0);
	event->isRecurring = IsDlgButtonChecked(hDlg, IDC_CHECK_RECURRING) == BST_CHECKED;
	event->recurrenceInterval = GetSelectedReccurenceInterval(hDlg);
	event->ResetNotificationStates();

	if (event->recurrenceInterval == RecurrenceInterval::Custom)
	{
		GetDlgItemText(hDlg, IDC_EDIT_CUSTOM_DAYS, buffer, 32);
		event->customRecurrenceDays = _wtoi(buffer);
	}
}

bool EventDialog::ValidateFields(HWND hDlg)
{
	WCHAR buffer[1025];  // Buffer for reading text

	// Validate event name
	GetDlgItemText(hDlg, IDC_EDIT_NAME, buffer, MAX_EVENT_NAME_LENGTH + 1);
	if (wcslen(buffer) == 0)
	{
		MessageBox(hDlg, L"Назва події не може бути порожньою", L"Помилка створення події", MB_ICONERROR);
		return false;
	}

	SYSTEMTIME stDate, stTime;
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATEPICKER), &stDate);
	DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_TIMEPICKER), &stTime);

	FILETIME ftEvent, ftNow;
	stDate.wHour = stTime.wHour;
	stDate.wMinute = stTime.wMinute;

	// Check if the event date/time is in the past
	if (IsSystemTimeInPast(stDate))
	{
		MessageBox(hDlg, L"Дата/час події не можуть бути минулими.", L"Помилка створення події", MB_ICONERROR);
		return false;
	}

	// Check for recurrence and custom days validity
	if (IsDlgButtonChecked(hDlg, IDC_CHECK_RECURRING) &&
		GetSelectedReccurenceInterval(hDlg) == RecurrenceInterval::Custom)
	{
		GetDlgItemText(hDlg, IDC_EDIT_CUSTOM_DAYS, buffer, 32);
		if (_wtoi(buffer) <= 0)
		{
			MessageBox(hDlg, L"Власний інтервал повторення події повинен бути більшим за 0 (днів)", L"Помилка створення події", MB_ICONERROR);
			return false;
		}
	}

	return true;
}

RecurrenceInterval EventDialog::GetSelectedReccurenceInterval(HWND hDlg)
{
	return (RecurrenceInterval)(SendDlgItemMessage(hDlg, IDC_COMBO_INTERVAL, CB_GETCURSEL, 0, 0) + 1);
}


void EventDialog::UpdateCharCount(HWND hDlg, int controlId)
{
	int maxLen = 0;
	int staticId = 0;

	// Determine the maximum length and static label ID based on the control
	switch (controlId)
	{
	case IDC_EDIT_NAME:
		maxLen = MAX_EVENT_NAME_LENGTH;
		staticId = IDC_STATIC_NAME_COUNT;
		break;
	case IDC_EDIT_DESC:
		maxLen = MAX_EVENT_DESC_LENGTH;
		staticId = IDC_STATIC_DESC_COUNT;
		break;
	case IDC_EDIT_NOTES:
		maxLen = MAX_EVENT_NOTES_LENGTH;
		staticId = IDC_STATIC_NOTES_COUNT;
		break;
	default:
		return;
	}

	// Get the current length of the text in the field
	int len = GetWindowTextLength(GetDlgItem(hDlg, controlId));

	// Update the static label with the current length and max length
	WCHAR buffer[32];
	swprintf(buffer, 32, L"%d/%d", len, maxLen);
	SetDlgItemText(hDlg, staticId, buffer);
}


#pragma region Categories

void EventDialog::AddNewCategory(HWND hDlg)
{
	WCHAR buffer[MAX_EVENT_CATEGORY_LENGTH] = { 0 };

	// Show the input dialog to get the new category name
	if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INPUT_DIALOG),
		hDlg, InputCategoryDlgProc, (LPARAM)buffer) == IDOK)
	{
		// Try to add the category through AppConfig method
		if (AppConfig::AddCategory(buffer))
		{
			// If category is successfully added, update the combo box
			HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_CATEGORY);
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)buffer);
			SendMessage(hCombo, CB_SETCURSEL, AppConfig::GetCategoryCount() - 1, 0);
			ApplicationUserInterface::ApplicationUI::RefreshCategories();
		}
		else
		{
			// Show message if the category already exists or exceeds the max length
			MessageBox(hDlg,
				L"Не вдалося створити категорію. Переконайтесь що ім'я категорії унікальне та не перевищує максимальний допустимий ліміт символів (64).",
				L"Помилка створення категорії", MB_ICONERROR);
		}
	}
}

INT_PTR CALLBACK EventDialog::InputCategoryDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static WCHAR* pBuffer = NULL;

	switch (msg)
	{
	case WM_INITDIALOG:
		pBuffer = (WCHAR*)lParam;
		SendDlgItemMessage(hDlg, IDC_EDIT_CATEGORY, EM_SETLIMITTEXT, MAX_EVENT_CATEGORY_LENGTH, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT_CATEGORY, pBuffer, MAX_EVENT_CATEGORY_LENGTH);
			if (wcslen(pBuffer) == 0)
			{
				MessageBox(hDlg, L"Ім'я нової категорії не може бути порожнім. Будь-ласка введіть коректне ім'я категорії.", L"Помилка створення категорії", MB_ICONERROR);
				return TRUE;
			}
			EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

#pragma endregion


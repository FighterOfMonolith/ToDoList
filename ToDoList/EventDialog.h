#pragma once
#include <windows.h>
#include "Event.h"

class EventDialog
{
public:
	static bool Show(HWND hParent, Event* event, Event* originalEvent = nullptr);

private:
	static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK InputCategoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	static void OnInitDialog(HWND hDlg, Event* event);
	static void ConfigureStaticLabelsStyle(HWND hDlg);
	static void ClearResources(HWND hDlg);

	static void OnOK(HWND hDlg, Event* event);
	static bool ValidateFields(HWND hDlg);
	static void UpdateCharCount(HWND hDlg, int controlId);
	static RecurrenceInterval GetSelectedReccurenceInterval(HWND hDlg);

	static void AddNewCategory(HWND hDlg);
	static void UpdateDateTimeWarning(HWND hDlg);
};
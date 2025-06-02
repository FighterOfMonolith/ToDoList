#include "Event.h"
#include <windows.h>
#include <string>
#include <ctime>
#include "Utilities.h"

Event::Event() :
	name(L""),
	description(L""),
	notes(L""),
	category(L"General"),
	isCompleted(false),
	isPastDeadline(false),
	priority(EventPriority::Medium),
	isRecurring(false),
	recurrenceInterval(RecurrenceInterval::None),
	customRecurrenceDays(0),
	isReccurenceHandled(false),
	notificationStates{ NOTIFICATION_NONE, NOTIFICATION_NONE, NOTIFICATION_NONE }
{
	// Initialize creation date to current time
	GetLocalTime(&creationDate);

	// Initialize deadline to same as creation date initially
	deadline = creationDate;

	// Initialize GUID with a new unique value
	CoCreateGuid(&uniqueID);
}

void Event::UpdateStatus()
{
	isPastDeadline = IsSystemTimeInPast(deadline);
}

std::wstring Event::GetStatusString() const
{
	if (isCompleted) return L"Виконано";
	if (isPastDeadline) return L"Не виконано";
	return L"Виконується";
}

bool Event::ShouldNotify(NotificationType type) const
{
	if (isCompleted && !isPastDeadline)
		return false;

	return notificationStates[type] == NOTIFICATION_NONE;
}

void Event::MarkNotified(NotificationType type)
{
	for (int i = 0; i <= type; ++i)
	{
		notificationStates[i] = NOTIFICATION_SENT;
	}
}

void Event::ResetNotificationStates()
{
	for (int i = 0; i < NOTIFICATION_COUNT; i++)
	{
		notificationStates[i] = NOTIFICATION_NONE;
	}
}
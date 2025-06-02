#pragma once
#include <windows.h>
#include <string>

enum EventPriority
{
    Low = 0,
    Medium = 1,
    High = 2,
    Critical = 3
};

enum RecurrenceInterval
{
    None = 0,
    Daily = 1,
    Weekly = 2,
    Monthly = 3,
    Yearly = 4,
    Custom = 5
};

enum NotificationState
{
    NOTIFICATION_NONE = 0,      // No notification sent
    NOTIFICATION_SENT           // Notification was sent
};

enum NotificationType
{
    NOTIFICATION_24H = 0,
    NOTIFICATION_12H = 1,
    NOTIFICATION_1H = 2,
    NOTIFICATION_DEADLINE_PASSED = 3,
    NOTIFICATION_COUNT
};

struct Event
{
    std::wstring name;
    std::wstring description;
    std::wstring notes;
    std::wstring category;
    SYSTEMTIME creationDate;
    SYSTEMTIME deadline;
    bool isCompleted;
    bool isPastDeadline;
    EventPriority priority;

    // Recurrence settings
    bool isRecurring;
    RecurrenceInterval recurrenceInterval;
    bool isReccurenceHandled;
    int customRecurrenceDays;
    NotificationState notificationStates[NOTIFICATION_COUNT];

    // Unique identifier
    GUID uniqueID;

    Event();
    void UpdateStatus();
    std::wstring GetStatusString() const;
    bool ShouldNotify(NotificationType type) const;
    void MarkNotified(NotificationType type);
    void ResetNotificationStates();
};

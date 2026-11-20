#ifndef RECOVERYENGINE_H
#define RECOVERYENGINE_H

#include "StudySession.h"
#include "Task.h"
#include "StudentProfile.h"

#include <vector>

using namespace std;

class RecoveryEngine
{
private:
    struct RecoveryRequest
    {
        int taskId;
        int earliestDay;
        double requiredHours;
    };

    double totalRecoveryHours;

    static double getPendingHours(const StudySession& session);
    static int getLastAllowedDay(const Task& task);
    static int clampImportance(int importance);
    static Task* findTaskById(vector<Task>& tasks, int taskId);

    double getUsedHoursOnDay(
        int dayNumber,
        const vector<StudySession>& sessions
    ) const;

    double getAvailableHoursOnDay(
        int dayNumber,
        const StudentProfile& student,
        const vector<StudySession>& sessions
    ) const;

    double scheduleRecoveryHours(
        vector<StudySession>& sessions,
        const Task& task,
        const StudentProfile& student,
        int earliestDay,
        double requiredHours
    );

    double freeLowPriorityHours(
        vector<StudySession>& sessions,
        vector<Task>& tasks,
        double requiredHours,
        int earliestDay,
        int lastAllowedDay,
        int protectedTaskId
    );

public:
    RecoveryEngine();

    double calculateRemainingTime(const vector<StudySession>& sessions);

    void increasePriority(vector<Task>& tasks, int taskId);

    void createRecoverySession(
        vector<StudySession>& sessions,
        const Task& task,
        int dayNumber,
        double plannedHours
    );

    void postponeLowPriorityTasks(
        vector<StudySession>& sessions,
        vector<Task>& tasks
    );

    bool checkAvailableTime(
        int dayNumber,
        double requiredHours,
        const StudentProfile& student,
        const vector<StudySession>& sessions
    );

    void generateWarning(const StudentProfile& student);

    void recoverTask(
        vector<StudySession>& sessions,
        vector<Task>& tasks,
        StudentProfile& student
    );
};

#endif
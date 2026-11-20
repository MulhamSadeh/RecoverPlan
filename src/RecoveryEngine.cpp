#include "../include/RecoveryEngine.h"

#include <iostream>
#include <iomanip>

using namespace std;

const int PLANNING_DAYS = 7;
const double EPSILON = 0.000001;

RecoveryEngine::RecoveryEngine()
{
    totalRecoveryHours = 0.0;
}

double RecoveryEngine::getPendingHours(const StudySession& session)
{
    if (session.isMissed())
    {
        return session.getPlannedHours();
    }

    if (session.isDelayed())
    {
        return session.getDelayHours();
    }

    return 0.0;
}

int RecoveryEngine::getLastAllowedDay(const Task& task)
{
    int lastAllowedDay = task.getDaysRemaining();

    if (lastAllowedDay < 1)
    {
        lastAllowedDay = 1;
    }

    if (lastAllowedDay > PLANNING_DAYS)
    {
        lastAllowedDay = PLANNING_DAYS;
    }

    return lastAllowedDay;
}

int RecoveryEngine::clampImportance(int importance)
{
    if (importance < 1)
    {
        return 1;
    }

    if (importance > 10)
    {
        return 10;
    }

    return importance;
}

Task* RecoveryEngine::findTaskById(vector<Task>& tasks, int taskId)
{
    for (size_t i = 0; i < tasks.size(); i++)
    {
        if (tasks[i].getTaskId() == taskId)
        {
            return &tasks[i];
        }
    }

    return nullptr;
}

double RecoveryEngine::calculateRemainingTime(const vector<StudySession>& sessions)
{
    double remainingHours = 0.0;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        remainingHours += getPendingHours(sessions[i]);
    }

    return remainingHours;
}

void RecoveryEngine::increasePriority(vector<Task>& tasks, int taskId)
{
    Task* task = findTaskById(tasks, taskId);

    if (task == nullptr)
    {
        return;
    }

    int currentImportance = task->getImportance();

    int priorityBoost = 1;

    if (totalRecoveryHours >= 4.0)
    {
        priorityBoost = 2;
    }

    int updatedImportance = clampImportance(currentImportance + priorityBoost);

    task->setImportance(updatedImportance);
}

void RecoveryEngine::createRecoverySession(
    vector<StudySession>& sessions,
    const Task& task,
    int dayNumber,
    double plannedHours
)
{
    int newSessionId = static_cast<int>(sessions.size()) + 1;

    StudySession recoverySession(
        newSessionId,
        task.getTaskId(),
        task.getSubjectName(),
        task.getTaskName(),
        dayNumber,
        plannedHours
    );

    sessions.push_back(recoverySession);
}

double RecoveryEngine::getUsedHoursOnDay(
    int dayNumber,
    const vector<StudySession>& sessions
) const
{
    double usedHours = 0.0;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (sessions[i].getDayNumber() == dayNumber)
        {
            usedHours += sessions[i].getPlannedHours();
        }
    }

    return usedHours;
}

double RecoveryEngine::getAvailableHoursOnDay(
    int dayNumber,
    const StudentProfile& student,
    const vector<StudySession>& sessions
) const
{
    double dailyAvailableHours = student.getAvailableHours();

    if (dailyAvailableHours < 0)
    {
        dailyAvailableHours = 0;
    }

    double usedHours = getUsedHoursOnDay(dayNumber, sessions);
    double availableHours = dailyAvailableHours - usedHours;

    if (availableHours < 0)
    {
        return 0;
    }

    return availableHours;
}

bool RecoveryEngine::checkAvailableTime(
    int dayNumber,
    double requiredHours,
    const StudentProfile& student,
    const vector<StudySession>& sessions
)
{
    double availableHours = getAvailableHoursOnDay(
        dayNumber,
        student,
        sessions
    );

    return availableHours + EPSILON >= requiredHours;
}

double RecoveryEngine::scheduleRecoveryHours(
    vector<StudySession>& sessions,
    const Task& task,
    const StudentProfile& student,
    int earliestDay,
    double requiredHours
)
{
    double remainingHours = requiredHours;

    int lastAllowedDay = getLastAllowedDay(task);

    if (earliestDay < 1)
    {
        earliestDay = 1;
    }

    if (earliestDay > PLANNING_DAYS)
    {
        return remainingHours;
    }

    for (int dayNumber = earliestDay;
         dayNumber <= lastAllowedDay && remainingHours > EPSILON;
         dayNumber++)
    {
        double availableHours = getAvailableHoursOnDay(
            dayNumber,
            student,
            sessions
        );

        int activeSessions = 0;
        for (const StudySession& session : sessions)
        {
            if (session.getDayNumber() == dayNumber &&
                session.getPlannedHours() > EPSILON)
                ++activeSessions;
        }
        int remainingSlots = student.getSessionsPerDay() - activeSessions;

        if (availableHours <= EPSILON || remainingSlots <= 0)
        {
            continue;
        }

        const double sessionCap =
            student.getAvailableHours() / student.getSessionsPerDay();

        while (remainingHours > EPSILON &&
               availableHours > EPSILON && remainingSlots > 0)
        {
            double assignedHours = remainingHours;
            if (assignedHours > availableHours) assignedHours = availableHours;
            if (assignedHours > sessionCap) assignedHours = sessionCap;

            createRecoverySession(sessions, task, dayNumber, assignedHours);
            remainingHours -= assignedHours;
            availableHours -= assignedHours;
            --remainingSlots;
        }
    }

    return remainingHours;
}

double RecoveryEngine::freeLowPriorityHours(
    vector<StudySession>& sessions,
    vector<Task>& tasks,
    double requiredHours,
    int earliestDay,
    int lastAllowedDay,
    int protectedTaskId
)
{
    double freedHours = 0.0;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (freedHours >= requiredHours - EPSILON)
        {
            break;
        }

        int sessionDay = sessions[i].getDayNumber();

        if (sessionDay < earliestDay || sessionDay > lastAllowedDay)
        {
            continue;
        }

        if (sessions[i].getTaskId() == protectedTaskId)
        {
            continue;
        }

        if (sessions[i].getPlannedHours() <= EPSILON)
        {
            continue;
        }

        Task* task = findTaskById(tasks, sessions[i].getTaskId());

        if (task == nullptr)
        {
            continue;
        }

        if (task->getIsCompleted())
        {
            continue;
        }

        if (task->getImportance() > 4)
        {
            continue;
        }

        double remainingNeed = requiredHours - freedHours;
        double currentSessionHours = sessions[i].getPlannedHours();

        double hoursToFree = currentSessionHours;

        if (hoursToFree > remainingNeed)
        {
            hoursToFree = remainingNeed;
        }

        sessions[i].setPlannedHours(currentSessionHours - hoursToFree);

        freedHours += hoursToFree;
    }

    return freedHours;
}

void RecoveryEngine::postponeLowPriorityTasks(
    vector<StudySession>& sessions,
    vector<Task>& tasks
)
{
    freeLowPriorityHours(
        sessions,
        tasks,
        totalRecoveryHours,
        1,
        PLANNING_DAYS,
        -1
    );
}

void RecoveryEngine::generateWarning(const StudentProfile& student)
{
    if (totalRecoveryHours > 5.0 || student.getNumberOfMissedSessions() > 3)
    {
        cout << "\n[Warning]: High recovery workload. "
             << totalRecoveryHours
             << " hours need to be rescheduled."
             << endl;
    }
}

void RecoveryEngine::recoverTask(
    vector<StudySession>& sessions,
    vector<Task>& tasks,
    StudentProfile& student
)
{
    totalRecoveryHours = calculateRemainingTime(sessions);

    if (totalRecoveryHours <= EPSILON)
    {
        return;
    }

    vector<RecoveryRequest> recoveryRequests;

    int newMissedSessions = 0;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        double pendingHours = getPendingHours(sessions[i]);

        if (pendingHours <= EPSILON)
        {
            continue;
        }

        RecoveryRequest request;
        request.taskId = sessions[i].getTaskId();
        request.earliestDay = sessions[i].getDayNumber() + 1;
        request.requiredHours = pendingHours;

        recoveryRequests.push_back(request);

        if (sessions[i].isMissed())
        {
            newMissedSessions++;
            sessions[i].setPlannedHours(0);
        }
        else if (sessions[i].isDelayed())
        {
            double completedHours =
                sessions[i].getPlannedHours() - sessions[i].getDelayHours();

            if (completedHours < 0)
            {
                completedHours = 0;
            }

            sessions[i].setPlannedHours(completedHours);
        }
    }

    student.setNumberOfMissedSessions(
        student.getNumberOfMissedSessions() + newMissedSessions
    );

    for (size_t i = 0; i < recoveryRequests.size(); i++)
    {
        increasePriority(tasks, recoveryRequests[i].taskId);
    }

    double unscheduledRecoveryHours = 0.0;

    for (size_t i = 0; i < recoveryRequests.size(); i++)
    {
        Task* task = findTaskById(tasks, recoveryRequests[i].taskId);

        if (task == nullptr)
        {
            unscheduledRecoveryHours += recoveryRequests[i].requiredHours;
            continue;
        }

        double remainingHours = scheduleRecoveryHours(
            sessions,
            *task,
            student,
            recoveryRequests[i].earliestDay,
            recoveryRequests[i].requiredHours
        );

        if (remainingHours > EPSILON)
        {
            int lastAllowedDay = getLastAllowedDay(*task);

            freeLowPriorityHours(
                sessions,
                tasks,
                remainingHours,
                recoveryRequests[i].earliestDay,
                lastAllowedDay,
                task->getTaskId()
            );

            remainingHours = scheduleRecoveryHours(
                sessions,
                *task,
                student,
                recoveryRequests[i].earliestDay,
                remainingHours
            );
        }

        unscheduledRecoveryHours += remainingHours;
    }

    cout << "\n==========================================================" << endl;
    cout << "              Study Schedule Recovery Report              " << endl;
    cout << "==========================================================" << endl;

    cout << "Student: " << student.getName() << endl;

    cout << "Recovery Hours: "
         << fixed << setprecision(2)
         << totalRecoveryHours << " hrs" << endl;

    cout << "Missed Sessions: "
         << student.getNumberOfMissedSessions() << endl;

    if (unscheduledRecoveryHours > EPSILON)
    {
        cout << "Unscheduled Recovery Hours: "
             << fixed << setprecision(2)
             << unscheduledRecoveryHours << " hrs" << endl;
    }

    generateWarning(student);

    cout << "----------------------------------------------------------" << endl;
    cout << "Updated Schedule Plan:" << endl;
    cout << "----------------------------------------------------------" << endl;

    cout << left
         << setw(8) << "Day"
         << setw(15) << "Subject"
         << setw(20) << "Task"
         << setw(12) << "Hours"
         << endl;

    cout << "----------------------------------------------------------" << endl;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (sessions[i].getPlannedHours() > EPSILON)
        {
            cout << left
                 << "Day " << setw(4) << sessions[i].getDayNumber()
                 << setw(15) << sessions[i].getCourseName()
                 << setw(20) << sessions[i].getTaskName()
                 << fixed << setprecision(2)
                 << setw(12) << sessions[i].getPlannedHours()
                 << endl;
        }
    }

    cout << "==========================================================" << endl;
}
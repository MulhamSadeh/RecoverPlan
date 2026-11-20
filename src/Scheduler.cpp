#include "../include/Scheduler.h"

#include <algorithm>

using namespace std;

Scheduler::Scheduler()
{
    nextSessionId = 1;
    scheduleMode = 1;
}

void Scheduler::setScheduleMode(int mode)
{
    if (mode >= 1 && mode <= 5)
    {
        scheduleMode = mode;
    }
    else
    {
        scheduleMode = 1;
    }
}

double Scheduler::getRequiredHours(const Task& task)
{
    double requiredHours = task.getPredictedDuration();

    if (requiredHours <= 0)
    {
        requiredHours = task.getStudentEstimatedTime();
    }

    return requiredHours;
}

int Scheduler::getLastAllowedDay(const Task& task)
{
    const int planningDays = 7;

    int lastAllowedDay = task.getDaysRemaining();

    if (lastAllowedDay < 1)
    {
        lastAllowedDay = 1;
    }

    if (lastAllowedDay > planningDays)
    {
        lastAllowedDay = planningDays;
    }

    return lastAllowedDay;
}

bool Scheduler::compareTasks(const Task& firstTask, const Task& secondTask) const
{
    if (scheduleMode == 1)
    {
        if (firstTask.getDaysRemaining() != secondTask.getDaysRemaining())
        {
            return firstTask.getDaysRemaining() < secondTask.getDaysRemaining();
        }
    }
    else if (scheduleMode == 2)
    {
        if (firstTask.getImportance() != secondTask.getImportance())
        {
            return firstTask.getImportance() > secondTask.getImportance();
        }
    }
    else if (scheduleMode == 3)
    {
        if (firstTask.getDifficulty() != secondTask.getDifficulty())
        {
            return firstTask.getDifficulty() > secondTask.getDifficulty();
        }
    }
    else if (scheduleMode == 4)
    {
        double firstHours = getRequiredHours(firstTask);
        double secondHours = getRequiredHours(secondTask);

        if (firstHours != secondHours)
        {
            return firstHours < secondHours;
        }
    }
    else if (scheduleMode == 5)
    {
        if (firstTask.getDelayProbability() != secondTask.getDelayProbability())
        {
            return firstTask.getDelayProbability() > secondTask.getDelayProbability();
        }
    }

    if (firstTask.getDaysRemaining() != secondTask.getDaysRemaining())
    {
        return firstTask.getDaysRemaining() < secondTask.getDaysRemaining();
    }

    if (firstTask.getImportance() != secondTask.getImportance())
    {
        return firstTask.getImportance() > secondTask.getImportance();
    }

    if (firstTask.getDifficulty() != secondTask.getDifficulty())
    {
        return firstTask.getDifficulty() > secondTask.getDifficulty();
    }

    double firstTaskHours = getRequiredHours(firstTask);
    double secondTaskHours = getRequiredHours(secondTask);

    if (firstTaskHours != secondTaskHours)
    {
        return firstTaskHours > secondTaskHours;
    }

    return firstTask.getTaskId() < secondTask.getTaskId();
}

void Scheduler::buildSchedule(vector<Task>& tasks, const StudentProfile& student)
{
    const int PLANNING_DAYS = 7;
    const double EPSILON = 0.000001;

    scheduledSessions.clear();
    overflowTasks.clear();

    nextSessionId = 1;

    sort(
        tasks.begin(),
        tasks.end(),
        [this](const Task& firstTask, const Task& secondTask)
        {
            return compareTasks(firstTask, secondTask);
        }
    );

    double dailyAvailableHours = student.getAvailableHours();

    if (dailyAvailableHours < 0)
    {
        dailyAvailableHours = 0;
    }

    const int dailySessionLimit = student.getSessionsPerDay();
    const double maxSessionHours = dailyAvailableHours / dailySessionLimit;
    vector<double> remainingHoursPerDay(PLANNING_DAYS, dailyAvailableHours);
    vector<int> remainingSessionsPerDay(PLANNING_DAYS, dailySessionLimit);

    for (size_t taskIndex = 0; taskIndex < tasks.size(); taskIndex++)
    {
        if (tasks[taskIndex].getIsCompleted())
        {
            continue;
        }

        double requiredHours = getRequiredHours(tasks[taskIndex]);

        if (requiredHours <= 0)
        {
            continue;
        }

        int lastAllowedDay = getLastAllowedDay(tasks[taskIndex]);

        double availableHoursBeforeDeadline = 0;

        for (int dayIndex = 0; dayIndex < lastAllowedDay; dayIndex++)
        {
            // Unused hours do not imply another session slot is available.
            if (remainingSessionsPerDay[dayIndex] > 0)
                availableHoursBeforeDeadline += remainingHoursPerDay[dayIndex];
        }

        if (availableHoursBeforeDeadline <= EPSILON)
        {
            Task overflowTask = tasks[taskIndex];
            overflowTask.setStudentEstimatedTime(requiredHours);
            overflowTasks.push_back(overflowTask);

            continue;
        }

        double remainingTaskHours = requiredHours;

        for (int dayIndex = 0;
             dayIndex < lastAllowedDay && remainingTaskHours > EPSILON;
             dayIndex++)
        {
            while (remainingTaskHours > EPSILON &&
                   remainingHoursPerDay[dayIndex] > EPSILON &&
                   remainingSessionsPerDay[dayIndex] > 0)
            {
                double assignedHours = remainingTaskHours;

                if (assignedHours > remainingHoursPerDay[dayIndex])
                    assignedHours = remainingHoursPerDay[dayIndex];
                if (assignedHours > maxSessionHours)
                    assignedHours = maxSessionHours;

                StudySession session(
                nextSessionId,
                tasks[taskIndex].getTaskId(),
                tasks[taskIndex].getSubjectName(),
                tasks[taskIndex].getTaskName(),
                dayIndex + 1,
                assignedHours
            );

                scheduledSessions.push_back(session);

                nextSessionId++;
                remainingTaskHours -= assignedHours;
                remainingHoursPerDay[dayIndex] -= assignedHours;
                remainingSessionsPerDay[dayIndex]--;
            }
        }

        if (remainingTaskHours > EPSILON)
        {
            Task overflowTask = tasks[taskIndex];
            overflowTask.setStudentEstimatedTime(remainingTaskHours);
            overflowTasks.push_back(overflowTask);
        }
    }
}

vector<StudySession>& Scheduler::getScheduledSessions()
{
    return scheduledSessions;
}

vector<Task>& Scheduler::getOverflowTasks()
{
    return overflowTasks;
}

const vector<StudySession>& Scheduler::getScheduledSessions() const
{
    return scheduledSessions;
}

const vector<Task>& Scheduler::getOverflowTasks() const
{
    return overflowTasks;
}
void Scheduler::loadSessions(const vector<StudySession>& sessions, const vector<Task>& overflow)
{
    scheduledSessions = sessions;
    overflowTasks     = overflow;
    if (!sessions.empty())
        nextSessionId = sessions.back().getSessionId() + 1;
}
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Task.h"
#include "StudentProfile.h"
#include "StudySession.h"

#include <vector>

using namespace std;

class Scheduler
{
private:
    vector<StudySession> scheduledSessions;
    vector<Task> overflowTasks;

    int nextSessionId;
    int scheduleMode;

    bool compareTasks(const Task& firstTask, const Task& secondTask) const;

    static double getRequiredHours(const Task& task);
    static int getLastAllowedDay(const Task& task);

public:
    Scheduler();

    void setScheduleMode(int mode);

    void buildSchedule(vector<Task>& tasks, const StudentProfile& student);

    vector<StudySession>& getScheduledSessions();
    vector<Task>& getOverflowTasks();

    const vector<StudySession>& getScheduledSessions() const;
    const vector<Task>& getOverflowTasks() const;
    void loadSessions(const vector<StudySession>& sessions, const vector<Task>& overflow);
};

#endif
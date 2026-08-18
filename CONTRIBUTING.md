Contributing — Project 1

This file lays out our week-by-week plan and basic workflow so everyone knows what's due when and how we work together in this repo.

Due: Monday, September 7, 2026, 23:59

Milestones
Week 1 — now to Aug 24: Task 1 + setup
mmcopier — basic pthread_create/pthread_join, one thread per file copy. No locking needed here, so it's a good on-ramp to threads.
Makefile working (make all, make clean)
README started (build/run instructions)
Confirm the code compiles on titan / jupiter / saturn early, so we're not fighting server issues at the last minute

Goal by end of week: mmcopier fully working and tested against the sample source_dir.

Week 2 — Aug 24 to Aug 31: Task 2, Subtask 1
Design the shared queue (array + head/tail/count, capacity 20)
Get reader threads and writer threads running and touching the queue
It's fine if it's not thread-safe yet, or even has a bug — the goal this week is understanding the reader/writer + queue structure before adding synchronization

Goal by end of week: readers and writers moving data through the shared queue, even without proper locking yet.

Week 3 — Aug 31 to Sept 7: Task 2, Subtasks 2 & 3, plus testing
Add pthread_mutex around the queue (Subtask 2)
Replace busy-waiting/sleep() with pthread_cond_t (Subtask 3) — this is usually the hardest conceptual jump, so it helps to have the mutex version solid first
Run valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all on both programs
Test edge cases (empty file, tiny file, n=2, n=10)
Finish the README (task completion table, design notes, testing notes)
Prepare final submission files (.zip, .txt for Turnitin, worklog .pdf) per Section 7
Download the submitted zip and double-check its contents before the deadline

Goal by end of week: both programs complete, valgrind-clean, documented, and submitted with time to spare.

Workflow
Pull before you start working: git pull
Commit small, working chunks with clear messages (e.g. git commit -m "Add reader thread queue push logic")
Push regularly so the group can see progress: git push
If you hit a merge conflict, don't panic — flag it in the group chat and we'll sort it out together rather than force-pushing over someone's work
Update the Task Completion Status table in README.md whenever a task/subtask is finished

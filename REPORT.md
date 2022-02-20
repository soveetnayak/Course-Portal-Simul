# OSN
## Assignment 5
## Question 1

## To compile
```
gcc q1.c -lpthread
```
## To execute
```
./a.out
```

## Structs implemented

* A struct for each course storing all the details.

* A struct for all the students.

* A struct for storing information of all the labs and lab TAs.

## Threads

* Multiple threads for simulating all the courses and its tutorials.

* Threads for each student taking up the courses.

## Course thread

* If the course is not removed yet, we go inside the while loop to allocate a TA.

* If a TA is available, that is not all the TA's are occupied or not all the TA's are exhausted, we conduct a tutorial for the course after waiting for 1 second to let the student threads register for the tutorial. If all people in the lab have exhausted the limit on the number of times someone can accept a TAship and so, the lab will no longer send out any TAs.

* The course thread sleeps for 3 sec for the duration of tutorial.

* On completion of the tutorial, a signal is broadcasted to wake up all the threads taking up this tutorial that were waiting on a conditional variable.

* When all the TAs are exhausted, the course is removed and we exit the main while loop.


## Student thread

* The student thread sleeps until it's time for registration.

* If the current course preference is not removed, the student waits for a tutorial to occur.

* The student then gets a seat from the alloted slots in the tutorial.

* When the student attends the tutorial, conditional wait is used to remove busy waiting. The student then wakes up on getting a signal from the respective course.

* The student either chooses a course permanently if his/her probability is greater than a randomly generated float between 0 and 1. The student doesn't get a course if the courses are removed or the probability is less.

* The student then exits the simulation.

Note:
Case 1: If the implementation is conducting a tutorial with 0 students as well
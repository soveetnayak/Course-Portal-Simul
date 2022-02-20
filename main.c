#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

#define MAX_STRING_LENGTH 100
#define MAX_LABS 1000
#define MAX 1000
#define PROB 0.5

struct course_struct
{
    int id;
    char course_name[MAX_STRING_LENGTH];
    float interest;
    int course_max_slots;
    int alloted_slots;
    int occupied_slots;
    int num_of_labs;
    int lab_list[MAX_LABS];
    int ta_alloted;                      //-1 = TA not alloted
    int lab_id_alloted;                  //-1 = Lab not alloted
    char lab_alloted[MAX_STRING_LENGTH]; //Lab name
    int removed;                         //1 = removed
    int check_for_seats;                 //Check done
};

struct students_struct
{
    int id;
    float calibre;
    int preference[3];
    int reg_time;
    int curr_course;
    int final_course; //-2 = not assigned -1 = not interested
};
struct ta_struct
{
    int id;
    int lab_id;
    int occupied;
    int curr_course;
    int times;
};

typedef struct ta_struct ta;

struct lab_struct
{
    int id;
    char lab_name[MAX_STRING_LENGTH];
    int max_ta;
    int max_times;
    int expired;
    ta *tas;
};

typedef struct course_struct course;
typedef struct students_struct student;
typedef struct lab_struct lab;
int num_students;
int num_labs;
int num_courses;

time_t tic;
time_t toc;

struct thread_for_course
{
    course *mycourse;
};

struct thread_for_student
{
    student *mystudent;
};

void *student_function(void *input);
void *lab_function(void *input);
void *course_function(void *input);

pthread_mutex_t mutex_course[MAX];
pthread_cond_t cond_course[MAX];
pthread_cond_t cond_student[MAX];
pthread_mutex_t mutex_student[MAX];
pthread_mutex_t mutex_lab[MAX];

course courses[MAX];
student students[MAX];
lab labs_array[MAX];

int main()
{

    scanf("%d", &num_students);
    scanf("%d", &num_labs);
    scanf("%d", &num_courses);

    // student students[num_students];
    // lab labs[num_labs];
    // course courses[num_courses];

    pthread_t students_threads[num_students];
    pthread_t labs_threads[num_labs];
    pthread_t courses_threads[num_courses];

    for (int i = 0; i < num_courses; i++)
    {
        scanf("%s", courses[i].course_name);
        scanf("%f", &courses[i].interest);
        scanf("%d", &courses[i].course_max_slots);
        scanf("%d", &courses[i].num_of_labs);
        for (int j = 0; j < courses[i].num_of_labs; j++)
        {
            scanf("%d", &courses[i].lab_list[j]);
        }
        courses[i].removed = 0;
        courses[i].ta_alloted = -1;
        courses[i].occupied_slots = 0;
        courses[i].check_for_seats = 0;
    }

    for (int i = 0; i < num_students; i++)
    {
        students[i].id = i;
        scanf("%f", &students[i].calibre);
        for (int j = 0; j < 3; j++)
        {
            scanf("%d", &students[i].preference[j]);
        }
        scanf("%d", &students[i].reg_time);
        students[i].curr_course = students[i].preference[0];
        students[i].final_course = -2;
    }

    for (int i = 0; i < num_labs; i++)
    {
        labs_array[i].id = i;
        scanf("%s", labs_array[i].lab_name);
        scanf("%d", &labs_array[i].max_ta);
        scanf("%d", &labs_array[i].max_times);
        labs_array[i].expired = 0;
        labs_array[i].tas = (ta *)malloc(labs_array[i].max_ta * sizeof(ta));
        for (int j = 0; j < labs_array[i].max_ta; j++)
        {
            labs_array[i].tas[j].id = j;
            labs_array[i].tas[j].lab_id = labs_array[i].id;
            labs_array[i].tas[j].times = 0;
            labs_array[i].tas[j].curr_course = -1;
            labs_array[i].tas[j].occupied = 0;
        }
    }

    for (int i = 0; i < MAX; i++)
    {
        pthread_mutex_init(&mutex_student[i], NULL);
        pthread_mutex_init(&mutex_course[i], NULL);
        pthread_cond_init(&cond_course[i], NULL);
        pthread_cond_init(&cond_student[i], NULL);
        pthread_mutex_init(&mutex_lab[i], NULL);
    }

    tic = time(NULL);
    for (int i = 0; i < num_students; i++)
    {
        struct thread_for_student *thread_input = (struct thread_for_student *)(malloc(sizeof(struct thread_for_student)));
        thread_input->mystudent = &students[i];
        pthread_create(&students_threads[i], NULL, student_function, (void *)(thread_input));
    }

    for (int i = 0; i < num_courses; i++)
    {
        struct thread_for_course *thread_input = (struct thread_for_course *)(malloc(sizeof(struct thread_for_course)));
        thread_input->mycourse = &courses[i];
        pthread_create(&courses_threads[i], NULL, course_function, (void *)(thread_input));
    }

    // for (int i = 0; i < num_courses; i++)
    // {
    //     pthread_join(courses_threads[i], NULL);
    // }

    for (int i = 0; i < num_students; i++)
    {
        pthread_join(students_threads[i], NULL);
    }
}

void *course_function(void *input)
{
    course *mycourse = ((struct thread_for_course *)input)->mycourse;

    while (mycourse->removed == 0)
    {
        mycourse->alloted_slots = rand() % (mycourse->course_max_slots - 1);
        mycourse->alloted_slots += 1;

        printf(GREEN "Course %s has been allocated %d seats\n", mycourse->course_name, mycourse->alloted_slots);

        while (mycourse->ta_alloted == -1)
        {
            for (int i = 0; i < mycourse->num_of_labs; i++)
            {
                int curr_lab = mycourse->lab_list[i];
                pthread_mutex_lock(&mutex_lab[curr_lab]);

                if (labs_array[curr_lab].expired == 2)
                {
                    pthread_mutex_unlock(&mutex_lab[curr_lab]);
                    continue;
                }

                for (int j = 0; j < labs_array[curr_lab].max_ta; j++)
                {

                    if (labs_array[curr_lab].tas[j].occupied == 0 && labs_array[curr_lab].tas[j].times < labs_array[curr_lab].max_times)
                    {

                        mycourse->ta_alloted = labs_array[curr_lab].tas[j].id;
                        mycourse->lab_id_alloted = curr_lab;
                        strcpy(mycourse->lab_alloted, labs_array[curr_lab].lab_name);

                        labs_array[curr_lab].tas[j].occupied = 1;
                        labs_array[curr_lab].tas[j].curr_course = mycourse->id;
                        labs_array[curr_lab].tas[j].times++;

                        printf(RED "TA %d from lab %s has been allocated to course %s for %d(st/nd/rd/th) TA ship\n", labs_array[curr_lab].tas[j].id, labs_array[curr_lab].lab_name, mycourse->course_name, labs_array[curr_lab].tas[j].times);
                        
                    }

                    if (j == labs_array[curr_lab].max_ta - 1 && labs_array[curr_lab].tas[j].times == labs_array[curr_lab].max_times)
                    {
                        labs_array[curr_lab].expired = 1;
                        mycourse->ta_alloted = -2;
                    }

                    if (mycourse->ta_alloted != -1)
                    {
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex_lab[curr_lab]);
                if(mycourse->ta_alloted != -1)
                {
                    break;
                }
            }
        }

        // if (labs_array[mycourse->lab_list[num_labs - 1]].expired == 2)
        // {
        //     mycourse->removed = 1;
        //     labs_array[mycourse->lab_id_alloted].tas[mycourse->ta_alloted].occupied = 0;
        //     pthread_mutex_unlock(&mutex_course[mycourse->id]);
        //     break;
        // }
        sleep(1);
        pthread_mutex_lock(&mutex_course[mycourse->id]);
        if (mycourse->ta_alloted >= 0)
        {

            printf(RED "Tutorial has started for Course %s with %d seats filled out of %d\n", mycourse->course_name, mycourse->occupied_slots, mycourse->alloted_slots);

            sleep(3); // Running tutorial for 3 seconds
            pthread_cond_broadcast(&cond_course[mycourse->id]);
            printf(RED "TA %d from lab %s has completed the tutorial and left the course %s\n", mycourse->ta_alloted, mycourse->lab_alloted, mycourse->course_name);
        }
        if (labs_array[mycourse->lab_id_alloted].expired == 1)
        {
            printf(MAGENTA "Lab %s no longer has students available for TA ship\n", labs_array[mycourse->lab_id_alloted].lab_name);
            labs_array[mycourse->lab_id_alloted].expired = 2;
        }

        if (labs_array[mycourse->lab_list[num_labs - 1]].expired == 2)
        {
            mycourse->removed = 1;
            labs_array[mycourse->lab_id_alloted].tas[mycourse->ta_alloted].occupied = 0;
            pthread_mutex_unlock(&mutex_course[mycourse->id]);
            break;
        }
        //int to_check_removed = 0;
        // for (int i = 0; i < mycourse->num_of_labs; i++)
        // {
        //     int curr_lab = mycourse->lab_list[i];
        //     if (labs_array[curr_lab].expired == 2)
        //     {
        //         to_check_removed += 1;
        //     }
        // }
        if (labs_array[mycourse->lab_id_alloted].expired == 1)
        {
            mycourse->ta_alloted = -2;
            printf(MAGENTA "Lab %s no longer has students available for TA ship\n", labs_array[mycourse->lab_id_alloted].lab_name);
            labs_array[mycourse->lab_id_alloted].expired = 2;
        }
        //Reset Variables
        mycourse->check_for_seats = 0;

        labs_array[mycourse->lab_id_alloted].tas[mycourse->ta_alloted].occupied = 0;

        mycourse->ta_alloted = -1;
        mycourse->lab_id_alloted = -1;
        mycourse->occupied_slots = 0;
        mycourse->alloted_slots = 0;
        pthread_mutex_unlock(&mutex_course[mycourse->id]);
    }
    pthread_cond_broadcast(&cond_course[mycourse->id]);
    printf(MAGENTA "Course %s doesn’t have any TA’s eligible and is removed from course offerings\n", mycourse->course_name);
    return NULL;
}

void *student_function(void *input)
{
    student *mystudent = ((struct thread_for_student *)input)->mystudent;
    time_t toc;
    toc = time(NULL);
    sleep(mystudent->reg_time);

    printf(YELLOW "Student %d has filled in preferences for course registration\n", mystudent->id);

    while (mystudent->final_course == -2)
    {
        if (courses[mystudent->curr_course].removed == 1)
        {
            if (mystudent->curr_course == mystudent->preference[2])
            {
                mystudent->final_course = -1;
                break;
            }
            for (int i = 0; i < 2; i++)
            {
                if (mystudent->preference[i] == mystudent->curr_course)
                {
                    printf(CYAN "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n", mystudent->id, courses[mystudent->preference[i]].course_name, i, courses[mystudent->preference[i + 1]].course_name, i + 1);
                    mystudent->curr_course = mystudent->preference[i + 1];
                    break;
                }
            }
        }

        if (courses[mystudent->curr_course].ta_alloted != -1 && courses[mystudent->curr_course].occupied_slots < courses[mystudent->curr_course].alloted_slots)
        {
            pthread_mutex_lock(&mutex_course[mystudent->curr_course]);
            if (courses[mystudent->curr_course].occupied_slots < courses[mystudent->curr_course].alloted_slots)
            {
                courses[mystudent->curr_course].occupied_slots++;
                printf(YELLOW "Student %d has been allocated a seat in course %s\n", mystudent->id, courses[mystudent->curr_course].course_name);
            }
            else
            {
                pthread_mutex_unlock(&mutex_course[mystudent->curr_course]);
                continue;
            }
            while (courses[mystudent->curr_course].removed == 0 && courses[mystudent->curr_course].ta_alloted == -1)
            {
                pthread_cond_wait(&cond_course[mystudent->curr_course], &mutex_course[mystudent->curr_course]);
            }

            // pthread_cond_wait(&cond_course[mystudent->curr_course], &mutex_course[mystudent->curr_course]);
            pthread_mutex_unlock(&mutex_course[mystudent->curr_course]);

            if (courses[mystudent->curr_course].removed == 1)
            {
                continue;
            }

            float prob = courses[mystudent->curr_course].interest * mystudent->calibre;
            //printf("Probabilit %f %s\n", prob, courses[mystudent->curr_course].course_name);
            if (prob > (float)rand() / (float)RAND_MAX)
            {
                mystudent->final_course = mystudent->curr_course;
                break;
            }
            else
            {
                printf(YELLOW "Student %d has withdrawn from course %s\n", mystudent->id, courses[mystudent->curr_course].course_name);
                if (mystudent->curr_course == mystudent->preference[2])
                {
                    mystudent->final_course = -1;
                    break;
                }
                for (int i = 0; i < 2; i++)
                {
                    if (mystudent->preference[i] == mystudent->curr_course)
                    {
                        printf(CYAN "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n", mystudent->id, courses[mystudent->preference[i]].course_name, i, courses[mystudent->preference[i + 1]].course_name, i + 1);
                        mystudent->curr_course = mystudent->preference[i + 1];
                        break;
                    }
                }
            }
        }
    }
    if (mystudent->final_course != -1)
    {
        printf(RESET "Student %d has selected course %s permanently\n", mystudent->id, courses[mystudent->final_course].course_name);
    }
    else
    {
        printf(RESET "Student %d couldn’t get any of his preferred courses\n", mystudent->id);
    }
    return NULL;
}

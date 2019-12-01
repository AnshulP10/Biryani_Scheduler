#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<time.h>
#include<limits.h>
#include<sys/shm.h>
#include<unistd.h>

int M, N, K, num_students;
pthread_mutex_t mutex_chefs[1000], mutex_tables[1000];
pthread_t cheftids[1000], tabletids[1000], studenttids[1000];

struct chef{
    int ind;
    int time;
    int num_portion_each;
    int num_vessel_each;
} chefs[1000];
struct table{
    int ind;
    int num_slots;
    int available_portion;
} tables[1000];
struct student{
    int ind;
    int status;
} students[1000];


int generator(int left, int right){
    int num = rand()%(right - left + 1);
    num+=left;
    return num;
}

void biryani_ready(int ind){
    while(chefs[ind].num_vessel_each){
        int j = N;
        while(j--){
            if(!pthread_mutex_trylock(&mutex_tables[N-j])){
                if(!tables[N-j].available_portion){
                    printf("Robot Chef %d is refilling Serving Container of Serving Table %d\n", chefs[ind].ind, tables[N-j].ind);
                    sleep(1);
                    chefs[ind].num_vessel_each--;
                    tables[N-j].available_portion = chefs[ind].num_portion_each;
                    printf("Serving Container of Table %d is refilled by Robot Chef %d; Table %d resuming serving now\n", tables[N-j].ind, chefs[ind].ind, tables[N-j].ind);
                    sleep(1);
                }
                pthread_mutex_unlock(&mutex_tables[N-j]);
                if(!chefs[ind].num_vessel_each)
                    break;
            }
        }
    }
    printf("All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now.\n", chefs[ind].ind);
}

void *chef_init(void *args){
    int ind = *(int *)args;
    int check = 0;
    while(!check){
        if(num_students){
            chefs[ind].time = generator(2, 5);
            chefs[ind].num_vessel_each = generator(1, 10);
            printf("Robot Chef %d is preparing %d vessels of Biryani\n",chefs[ind].ind, chefs[ind].num_vessel_each);
            sleep(chefs[ind].time);
            chefs[ind].num_portion_each = generator(25, 50);
            printf("Robot Chef %d has prepared %d vessels of Biryani. Waiting for all the vessels to be emptied to resume cooking\n", chefs[ind].ind, chefs[ind].num_vessel_each); 
            sleep(1);
            biryani_ready(ind);
        }
        else
            break;
    }
}

void ready_to_serve_table(int ind){
    int check = 0;
    while(!check){
        if(!pthread_mutex_trylock(&mutex_tables[ind])){
            if(!tables[ind].num_slots || !num_students){
                printf("Serving table %d entering Serving Phase\n", tables[ind].ind);
                pthread_mutex_unlock(&mutex_tables[ind]);
                break;
            }
            pthread_mutex_unlock(&mutex_tables[ind]);
        }
    }
}

void *table_init(void *args){
    int ind = *(int *)args;
    int check = 0;
    while(!check){
        if(num_students){
            if(tables[ind].available_portion){
                int num_slots = generator(1, 10);
                if(num_slots<=tables[ind].available_portion)
                    tables[ind].num_slots = num_slots;
                else
                    tables[ind].num_slots = tables[ind].available_portion;
                printf("Table %d is ready to serve in %d slots\n", tables[ind].ind, tables[ind].num_slots);
                sleep(1);
                ready_to_serve_table(tables[ind].ind);
            }
        }
        else
            check = 1;
    }
}

void student_in_slot(int ind, int j){
    tables[j].available_portion--;
    if(tables[j].available_portion==0)
        printf("Serving Container of Table %d is empty, waiting for refill\n", tables[j].ind);
    num_students--;
    tables[j].num_slots--;
    pthread_mutex_unlock(&mutex_tables[j]);
    while(num_students && tables[j].num_slots){    
    }
    printf("Student %d on serving table %d has been served\n", students[ind].ind, tables[j].ind);
}

void wait_for_slots(int ind){
    printf("Student %d is waiting to be allocated a slot on the serving table\n", ind);
    int check = 0;
    while(!check){
        int j = N;
        while(j--){
            if(!pthread_mutex_trylock(&mutex_tables[N-j])){
                if(tables[N-j].num_slots){
                    check = 1;
                    printf("Student %d assigned a slot on the serving table %d and waiting to be served.\n", ind, N-j);
                    student_in_slot(ind, N-j);
                    pthread_mutex_unlock(&mutex_tables[N-j]);
                    break;
                }
                pthread_mutex_unlock(&mutex_tables[N-j]);
            }
        }
    }
}

void *student_init(void *args){
    int ind = *(int *)args;
    printf("Student %d has arrived.\n", ind);
    wait_for_slots(ind);
}

int main(void){
    srand(time(NULL));
    printf("Give the number of Robot Chefs: ");
    scanf("%d", &M);
    printf("Give the number of Serving Tables: ");
    scanf("%d", &N);
    printf("Give the number of Students: ");
    scanf("%d", &K);
    num_students = K;
    if(!M || !N || !K){
        printf("Simulation over\n");
        return 0;
    }
    
    int i = M;
    while(i--){
        chefs[M-i].ind = M-i;
        usleep(100);
        pthread_create(&cheftids[M-i], NULL, chef_init, (void *)&chefs[M-i].ind);
    }
    
    sleep(1);
    i = N;
    while(i--){
        tables[N-i].ind = N-i;
        usleep(100);
        pthread_create(&tabletids[N-i], NULL, table_init, (void *)&tables[N-i].ind);
    }
    sleep(1);
    i = K;
    while(i--){
        students[K-i].ind = K-i;
        usleep(100);
        pthread_create(&studenttids[K-i], NULL, student_init, (void *)&students[K-i].ind);
    }

    i = K;
    while(i--)
        pthread_join(studenttids[K-i],NULL);
    printf("Simulation over\n");
    return 0;
}

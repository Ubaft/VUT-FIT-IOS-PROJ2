// proj2.c
// Řešení IOS-PROJ2, 6.5.2020
// Autor: Filip Osvald, FIT
// Přeloženo: gcc 7.5.0
// synchronizace procesu

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<semaphore.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<limits.h>
#include<string.h>
#include<time.h>

void judge(int jg, int jt);
void generator(int pi, int ig, int it);
int communism();
void genocide();
void immigrant(int id, int it);
FILE *fp;
sem_t *judge_out = NULL;
sem_t *mutex = NULL;
sem_t *confirm = NULL;
sem_t  *all_registered = NULL;
sem_t *write_to_file = NULL;
int *order_id = NULL;
int *undecided = NULL;
int *registered = NULL;
int *inside = NULL;
int *judge_in = NULL;
int *done = NULL;

int main(int argc, char *argv[])
{
    //kontrolujeme argumenty
    if(argc != 6)
    {
        fprintf(stderr, "invalid number of arguments\n");
        return 1;
    }
    char *end;
    int status = 0;

    int args[6];
    args[1] = strtol(argv[1], &end, 10);
    if (*end != '\0')
    {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }
    if(args[1] < 1)
    {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }
    for(int i = 2; i < 6; i++)
    {
        args[i] = strtol(argv[i], &end, 10);
        if (*end != '\0')
        {
            fprintf(stderr, "Invalid arguments\n");
            return 1;
        }
        if(args[i] < 0 || args[i] > 2000)
        {
            fprintf(stderr, "Invalid arguments\n");
            return 1;
        }
    }
    //po zkontrolovani argumentu inicializujeme potrebne zdroje
    int ret = communism();
    if(ret != 0)
    {
        genocide();
        return 1;   
    }
    *done = strtol(argv[1], &end, 10);
    //vytvorime proces soudce
    pid_t id = fork();
    if(id == 0)
    {
        judge(strtol(argv[3], &end, 10), strtol(argv[5], &end, 10));
    }
    //vytvorime proces generatoru
    pid_t id2 = fork();
    if (id2 == 0)
    {
        generator(strtol(argv[1], &end, 10), strtol(argv[2], &end, 10), strtol(argv[4], &end, 10));
    }
    //decka se flakaj
    while(wait(&status) > 0);
    genocide();
    return 0;
}
//proces soudce je presmerovan do teto funkce
void judge(int jg, int jt)
{
    srand(time(0));
    //dokuk vsichni imigranti nejsou vyrizeni
    while (*done > 0)
    {
        //zpusob spani
        if(jg != 0)
            usleep((rand() % (jg + 1))*1000);
        sem_wait(judge_out);
        sem_wait(mutex);
        sem_wait(write_to_file);
        fprintf(fp, "%d    : JUDGE   : wants to enter\n", (*order_id)++);
        sem_post(write_to_file);
        sem_wait(write_to_file);
        fprintf(fp, "%d    : JUDGE   : enters    :%d  :%d  :%d\n", (*order_id)++, *undecided, *registered, *inside);
        sem_post(write_to_file);
        //hlidma pritomnost soudce
        *judge_in = 1;
        if(*undecided > *registered)
        {
            sem_wait(write_to_file);
            fprintf(fp, "%d    : JUDGE   : waits for imm    :%d  :%d  :%d\n", (*order_id)++, *undecided, *registered, *inside);
            sem_post(write_to_file);
            sem_post(mutex);
            sem_wait(all_registered);
        }
        sem_wait(write_to_file);
        fprintf(fp, "%d    : JUDGE   : starts confirmation    :%d  :%d  :%d\n", (*order_id)++, *undecided, *registered, *inside);
        sem_post(write_to_file);
        //zase spime        
        if(jt != 0)
            usleep((rand() % (jt + 1))*1000);

        sem_wait(write_to_file);
        fprintf(fp, "%d    : JUDGE   : ends confirmation    :0  :0  :%d\n", (*order_id)++, *inside);
        sem_post(write_to_file);
        //potvrdime vsem registrovanym
        for (int i = 0; i < *registered; i++)
        {
            sem_post(confirm);
        }
        *undecided = *registered = 0;
        sem_wait(write_to_file);
        fprintf(fp, "%d    : JUDGE   : leaves    :%d  :%d  :%d\n", (*order_id)++, *undecided, *registered, *inside);
        sem_post(write_to_file);
        *judge_in = 0;
        sem_post(mutex);
        sem_post(judge_out);
    }
    //soudce konci
    sem_wait(write_to_file);
    fprintf(fp, "%d    : JUDGE    : finishes\n", (*order_id)++);
    sem_post(write_to_file);
    exit(0);
}
//proces imigranta je vzdy presmerovan do teto funkce
void immigrant(int id, int it)
{
    
    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : starts\n", (*order_id)++, id);
    sem_post(write_to_file);
    sem_wait(judge_out);
    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : enters  :%d  :%d  :%d\n", (*order_id)++, id, ++(*undecided), *registered, ++(*inside));
    sem_post(write_to_file);
    sem_post(judge_out);
    sem_wait(mutex);
    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : checks  :%d  :%d  :%d\n", (*order_id)++, id, *undecided, ++(*registered), (*inside));
    sem_post(write_to_file);
    //poslat signal soudci pokud jsou vsichni registrovani
    if(*judge_in == 1 && *undecided == *registered)
        sem_post(all_registered);
    else
    {
        sem_post(mutex);
    }
    //pockat na soudcovo rozhodnuti
    sem_wait(confirm);
    (*done)--;
    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : wants certificate  :%d  :%d  :%d\n", (*order_id)++, id, *undecided, (*registered), (*inside));
    sem_post(write_to_file);
    
    if(it != 0)
        usleep((rand() % (it + 1))*1000);

    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : got certificate  :%d  :%d  :%d\n", (*order_id)++, id, *undecided, (*registered), (*inside));
    sem_post(write_to_file);
    sem_wait(judge_out);
    sem_wait(write_to_file);
    fprintf(fp, "%d    : IMM %d  : leaves  :%d  :%d  :%d\n", (*order_id)++, id, *undecided, (*registered), --(*inside));
    sem_post(write_to_file);
    sem_post(judge_out);
    //immigrant umira
    exit(0);
}
//funkce ktera generuje PI procesu imigrantu
void generator(int pi, int ig, int it)
{
    int status = 0;
    srand(time(0));
    for(int i = 0; i < pi; i++)
    {
        if(ig != 0)
            usleep((rand() % (ig + 1))*1000);
        pid_t id_im = fork();
        if (id_im == 0)
        {
            immigrant(i+1, it);
        }
    }
    //decka se flakaj
    while(wait(&status) > 0);
    exit(0);
}
//Funkce inicializuje globalni promene a semafory
int communism()
{
    //semafory
    if((judge_out = sem_open("/xosval04.ios.semaphore1", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "cant open semaphore\n");
        return 1;
    }
    if((mutex = sem_open("/xosval04.ios.semaphore2", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "cant open semaphore\n");
        return 1;
    }
    if((confirm = sem_open("/xosval04.ios.semaphore3", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "cant open semaphore\n");
        return 1;
    }
    if((all_registered = sem_open("/xosval04.ios.semaphore4", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "cant open semaphore\n");
        return 1;
    }
    if((write_to_file = sem_open("/xosval04.ios.semaphore5", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "cant open semaphore\n");
        return 1;
    }
    //globalni promene
    judge_in = mmap(NULL, sizeof(*(inside)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    order_id = mmap(NULL, sizeof(*(order_id)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    undecided = mmap(NULL, sizeof(*(undecided)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    registered = mmap(NULL, sizeof(*(registered)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    inside = mmap(NULL, sizeof(*(inside)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    done = mmap(NULL, sizeof(*(done)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //soubor s vystupem
    fp = fopen("proj2.out", "w");
    (*order_id)++;
    setbuf(fp, NULL);
    return 0;
}
//funkce uklizi semafory a globalni promene
void genocide()
{
    //uklizeni semaforu
    sem_close(judge_out);
    sem_unlink("/xosval04.ios.semaphore1");
    sem_close(mutex);
    sem_unlink("/xosval04.ios.semaphore2");
    sem_close(confirm);
    sem_unlink("/xosval04.ios.semaphore3");
    sem_close(all_registered);
    sem_unlink("/xosval04.ios.semaphore4");
    sem_close(write_to_file);
    sem_unlink("/xosval04.ios.semaphore5");

    //uklizeni globalnich promenych
    munmap(judge_in, sizeof(judge_in));
    munmap(order_id, sizeof(order_id));
    munmap(undecided, sizeof(undecided));
    munmap(registered, sizeof(registered));
    munmap(inside, sizeof(inside));
    munmap(done, sizeof(done));
    fclose(fp);
}
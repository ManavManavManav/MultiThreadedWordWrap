#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"
#include <libgen.h>

bool brokeWidth = false;

Queue dirqueue = {NULL, NULL};
Queue filqueue = {NULL, NULL};

int width;

void removeExtraSpace(char *string)
{
    int i, j;
    int len = strlen(string);
    for (i = 0; i < len; i++)
    {
        if (string[0] == ' ')
        {
            for (i = 0; i < (len - 1); i++)
                string[i] = string[i + 1];
            string[i] = '\0';
            len--;
            i = -1;
            continue;
        }
        if (string[i] == ' ' && string[i + 1] == ' ')
        {
            for (j = i; j < (len - 1); j++)
            {
                string[j] = string[j + 1];
            }
            string[j] = '\0';
            len--;
            i--;
        }
    }
}

char *strtoke(char *str, const char *delim)
{
    static char *start = NULL;
    char *token = NULL;
    if (str)
        start = str;
    if (!start)
        return NULL;
    token = start;
    start = strpbrk(start, delim);
    if (start)
        *start++ = '\0';
    return token;
}

void wordWrap(char *ogFile, char *wrapFileName)
{
    
    FILE *file=fopen(ogFile,"r");
    int counter = 0;
    FILE *destF;
    if (wrapFileName == NULL)
    {
        destF = stdout;
    }
    else
    {
        destF = fopen(wrapFileName, "w+");
    }
    int lastLineWasNotNewLine = 1;
    // return;
    FILE *fp = file;
    char *temp;
    char *line = (char *)calloc(sizeof(char)*1000,1000);
    if (fp)
    {
        while (1)
        {
            char *c = NULL;
            while ((c = fgets(line, 1000, fp)) != NULL)
            {
                line = strncpy(line, line, strlen(line - 1));
                removeExtraSpace(line);

                if (line[0] == '\n')
                {
                    if (lastLineWasNotNewLine == 1)
                    {
                        fprintf(destF, "\n\n");
                        lastLineWasNotNewLine = 0;
                    }
                    counter = 0;
                    break;
                }
                else
                {
                    lastLineWasNotNewLine = 1;
                }

                line[strcspn(line, "\n")] = 0;

                char *ed2;
                char *word = strtok_r(line, " ", &ed2);
                while (word != NULL)
                {
                    if (strlen(word) > width)
                    {
                        brokeWidth = true;
                    }
                    if (counter + strlen(word) + 1 > width)
                    {
                        fprintf(destF, "\n");
                        counter = 0;
                    }
                    if (counter != 0)
                    {
                        fprintf(destF, " ");
                        counter++;
                    }
                    counter += strlen(word);
                    fprintf(destF, "%s", word);

                    word = strtok_r(NULL, " ", &ed2);

                    // if(strlen(word)==0){
                    //     return;
                    // }
                }
            }

            if (c == NULL)
                break;
        }

        fclose(fp);
    }
    // else
    // {
    //     printf("Error opening file...");
    // }
}

void recursiveSingleDir(char *dirName)
{
    DIR *dir;
    struct dirent *dent;

    if (!(dir = opendir(dirName)))
    {
        printf("dirnotfound\n");
        return;
    }
    else
    {
        // printf("founddir\n");
    }

    while ((dent = readdir(dir)) != NULL)
    {
        char *dname = dent->d_name;

        if (dname[0] == '.')
        {
            continue;
        }
        else if (dent->d_type == DT_DIR)
        {
            continue;
        }
        else if(strncmp(dname,"wrap.",5)!=0)
        {
            char *wrpName = (char *)malloc(sizeof(dname) + 6 + sizeof(dirName));
            char *newName = (char *)malloc(sizeof(dname) + sizeof(dirName));
            strcat(wrpName, dirName);
            strcat(wrpName, "/wrap.");
            strcat(wrpName, dname);

            strcat(newName, dirName);
            strcat(newName, "/");
            strcat(newName, dname);

            wordWrap(newName, wrpName);
        }
    }
}

int dirTCount=0;
int filTCount=0;

sem_t semOne;
sem_t semTwo;
pthread_mutex_t dirLock;
pthread_mutex_t dtcLock;
pthread_mutex_t filLock;
pthread_mutex_t extraLock;

int activeThreads=0;
int activeWWTthreads=0;
int dirThreadsDone=0;

void *producer(void *args)
{
    while(true){
        sem_wait(&semOne);
	//printf("Wait Complete: -%ld-, posted!\n",pthread_self());
        pthread_mutex_lock(&dtcLock);
	activeThreads++;
        pthread_mutex_unlock(&dtcLock);
	pthread_mutex_lock(&dirLock);
        // pthread_mutex_unlock(&dtcLock);
        char *origin=dequeue(&dirqueue);
	pthread_mutex_unlock(&dirLock);
        if(origin==NULL){
            // printf("-----------%d\n",activeThreads);
            // pthread_mutex_lock(&dtcLock);
            // pthread_mutex_lock(&dirLock);
	    pthread_mutex_lock(&dtcLock);
	    pthread_mutex_lock(&dirLock);
            activeThreads--;
                if(is_empty(dirqueue)&&activeThreads<=0){
		    dirThreadsDone = 1;
                    // printf("ENDCASE 1\n");
                    for(int i=0;i<dirTCount;i++){
                        sem_post(&semOne);
                    }
                    pthread_mutex_unlock(&dirLock);
                    pthread_mutex_unlock(&dtcLock);
		    // printf("Ending Dir Thread [%ld]\n", pthread_self());
		    // fflush(stdout);
                    return NULL;
                }
            pthread_mutex_unlock(&dirLock);
            pthread_mutex_unlock(&dtcLock);
            continue;
            // if(activeThreads>0){
            //     pthread_mutex_unlock(&dtcLock);
            //     continue;
            // }
            // else{
            //     pthread_mutex_unlock(&dtcLock);
            //     return NULL;
            // }
        }
        //pthread_mutex_unlock(&dirLock);
        //pthread_mutex_unlock(&dtcLock);
        // printf("%s\n",origin);
        
        
        // printf("%ld\n",pthread_self());

        struct dirent *dent;
        DIR *dir=opendir(origin);
        while((dent=readdir(dir))!=NULL){
            char *path=(char*)malloc((sizeof(char))*90);
            char *entryName=dent->d_name;

            if(entryName[0]=='.'){
                continue;
            }

            if(dent->d_type==4){
                strcpy(path, origin);
                strcat(path, "/");
                strcat(path, entryName);
                // printf("diectory Found: %s-----\n", path);
                pthread_mutex_lock(&dirLock);
                dirqueue=enqueue(dirqueue,path);
                pthread_mutex_unlock(&dirLock);
                // sleep(1);
                sem_post(&semOne);
            }
            else if(strncmp(entryName,"wrap.",5)!=0){
                strcpy(path, origin);
                strcat(path, "/");
                strcat(path, entryName);
                // printf("File Found [%ld]: %s-----\n", pthread_self(), path);
               pthread_mutex_lock(&filLock);
               filqueue=enqueue(filqueue,path);
               pthread_mutex_unlock(&filLock);
              	sem_post(&semTwo);
            }
        }
        pthread_mutex_lock(&dtcLock);
        pthread_mutex_lock(&dirLock);
        activeThreads--;
        // printf("%d\n",activeThreads);
        if(is_empty(dirqueue)&&activeThreads==0){
	    dirThreadsDone = 1;
            // printf("ENDCASE 2\n");
            for(int i=0;i<dirTCount;i++){
                sem_post(&semOne);
            }
	    // printf("Ending Dir Thread [%ld]\n", pthread_self());
	    fflush(stdout);
            pthread_mutex_unlock(&dirLock);
            pthread_mutex_unlock(&dtcLock);
            return NULL;
        }
        pthread_mutex_unlock(&dirLock);
        pthread_mutex_unlock(&dtcLock);
        sleep(1);
    }

}

void *consumer(void *args)
{
    while(true){
      	sem_wait(&semTwo);
        // printf("FTHREAD Wait Complete: -%ld-, posted!\n",pthread_self());
	fflush(stdout);
        pthread_mutex_lock(&extraLock);
        activeWWTthreads++;
	pthread_mutex_unlock(&extraLock);
        pthread_mutex_lock(&filLock);
        char *fil=dequeue(&filqueue);
	pthread_mutex_unlock(&filLock);
        if(fil==NULL)
        {
	    pthread_mutex_lock(&extraLock);
	    pthread_mutex_lock(&filLock);
	    //pthread_mutex_lock(&dtcLock);
	    //pthread_mutex_lock(&dirLock);
            activeWWTthreads--;
            if(dirThreadsDone != 0 && is_empty(filqueue)&&activeWWTthreads<=0){
                // printf("FENDCASE 1\n");
		fflush(stdout);

                for(int i=0;i<filTCount;i++){
                    sem_post(&semTwo);
                }
		// printf("Ending File Thread [%ld]\n", pthread_self());
		fflush(stdout);
                //pthread_mutex_unlock(&dirLock);
		//pthread_mutex_unlock(&dtcLock);
                pthread_mutex_unlock(&filLock);
                pthread_mutex_unlock(&extraLock);
                return NULL;
            }
            //pthread_mutex_unlock(&dirLock);
	    //pthread_mutex_unlock(&dtcLock);
            pthread_mutex_unlock(&filLock);
            pthread_mutex_unlock(&extraLock);
            continue;
        }
        // printf("%s\n",fil);
        //wwcodehere

        char *dirC,*baseC,*baseName,*dirName2;
        dirC=strdup(fil);
        baseC=strdup(fil);
        dirName2=dirname(dirC);
        baseName=basename(baseC);
	int dirNameLen = strlen(dirName2);
	int fileNameLen = strlen(baseName);
        char *ogName=(char*)malloc(dirNameLen + fileNameLen + 2);
        char *wrapName=(char*)malloc(dirNameLen + fileNameLen + 7);
        
	memcpy(ogName, dirName2, dirNameLen);
	memcpy(wrapName, dirName2, dirNameLen);

	ogName[dirNameLen] = '/';
	wrapName[dirNameLen] = '/';
	
	memcpy(ogName + dirNameLen + 1, baseName, fileNameLen);
	memcpy(wrapName + dirNameLen + 1, "wrap.", fileNameLen);
	memcpy(wrapName + dirNameLen + 6, baseName, fileNameLen);

	ogName[dirNameLen + fileNameLen + 1] = '\0';
	wrapName[dirNameLen + fileNameLen + 6] = '\0';

        //strcat(ogName,dirName2);
        //strcat(ogName,"/");
        //strcat(ogName,baseName);

        //strcat(wrapName,dirName2);
        //strcat(wrapName,"/wrap.");
        //strcat(wrapName,baseName);
        
        // printf("\n\n\nOG File name: %s\nWrap File Name: %s\n\n\n",ogName,wrapName);
	// fflush(stdout);
        wordWrap(ogName,wrapName);




	    pthread_mutex_lock(&extraLock);
	    pthread_mutex_lock(&filLock);
	    //pthread_mutex_lock(&dtcLock);
	    //pthread_mutex_lock(&dirLock);
            activeWWTthreads--;
            if(dirThreadsDone != 0 && is_empty(filqueue)&&activeWWTthreads<=0){
                // printf("FENDCASE 2\n");
		fflush(stdout);

                for(int i=0;i<filTCount;i++){
                    sem_post(&semTwo);
                }
		// printf("Ending File Thread [%ld]\n", pthread_self());
		fflush(stdout);
                //pthread_mutex_unlock(&dirLock);
		//pthread_mutex_unlock(&dtcLock);
                pthread_mutex_unlock(&filLock);
                pthread_mutex_unlock(&extraLock);
                return NULL;
            }
            pthread_mutex_unlock(&filLock);
            pthread_mutex_unlock(&extraLock);
	    
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ccccc=sem_init(&semOne,0,0);
    int c2=sem_init(&semTwo,0,0);
    // printf("%d-%d\n",ccccc,c2);
    if(ccccc!=0){
        perror("potatoto");
    }
    if(c2!=0){
        perror("potatoto");
    }
    if (pthread_mutex_init(&dtcLock, NULL) != 0) {
        perror("Failed initializing dtcLock\n");
    }
    if (pthread_mutex_init(&dirLock, NULL) != 0) {
        perror("Failed initializing dirLock\n");
    }
    if (pthread_mutex_init(&filLock, NULL) != 0) {
        perror("Failed initializing filLock\n");
    }
    if (pthread_mutex_init(&extraLock, NULL) != 0) {
        perror("Failed initializing extraLock\n");
    }
    
    int N = 1;
    int M = 1;
    char *reccheck=strstr(argv[1],"-r");
    
    if (argc == 3)
    {
        width = atoi(argv[1]);
        char *dirName = argv[2];

        struct stat bufr;
        stat(dirName, &bufr);

        if (S_ISDIR(bufr.st_mode))
        {
            recursiveSingleDir(dirName);
        }
        else if(strstr(argv[2],"/"))
        {
            wordWrap(dirName, NULL);
        }
        else{
            char* storageFile=(char*)malloc(sizeof(dirName)+6);
            strcpy(storageFile,"wrap.");
            strcat(storageFile,dirName);
            wordWrap(dirName,storageFile);
        }
    }
    else if(!reccheck){
        //doesnt have-r
        // printf("case3-%s\n",argv[1]);
        for(int i=2;i<argc-1;i++){
            struct stat bufr;
            stat(argv[i],&bufr);
            int exist=stat(argv[i],&bufr);
            if(exist!=-1){
                //does exist
                if(S_ISDIR(bufr.st_mode)){
                    recursiveSingleDir(argv[i]);
                }
                else{
                    //doo file stuff to use for straight wordwrap
                }

            }
        }
    }
    else
    {
        char *action = argv[1];
        int width = atoi(argv[2]);
        char *dirName = argv[3];

        struct stat bufr;
        stat(argv[3], &bufr);
        int exist = stat(argv[3], &bufr);
        if (exist == -1 || !S_ISDIR(bufr.st_mode))
        {
            // printf("%s\nCHECK DIR\n",dirName);
            return EXIT_FAILURE;
        }

        if (strlen(action) == 2)
        {
            N = 1;
            M = 1;
        }
        else if (true)
        {
            action = strtok(action, "-r");
            N = atoi(strtok(action, ","));
            action = strtok(NULL, ",");
            if (action == NULL)
            {
                M = 1;
            }
            else
            {
                M = N;
                N = atoi(action);
            }
        }
        // printf("N=%d\nM=%d\nWidth=%d\n", N, M, width);
        //------------------------------------------------------------

        pthread_t dirThreads[M];
        pthread_t filThreads[N];

        dirTCount=M;
        filTCount=N;
        dirqueue = enqueue(dirqueue, dirName);

        for (int i = 0; i < M; i++)
        {
            if (pthread_create(&dirThreads[i], NULL, &producer, (void*)&i) != 0)
            {
                perror("Failed to create thread");
            }
            if(i==0){
                sem_post(&semOne);
            }
        }
        for (int i = 0; i < N; i++)
        {
            if (pthread_create(&filThreads[i], NULL, &consumer, (void*)&i) != 0)
            {
                perror("Failed to create thread");
            }
        }

        for (int i = 0; i < M; i++)
        {
            if (pthread_join(dirThreads[i], NULL) != 0)
            {
                perror("Failed to join thread");
            }
        }
        for (int i = 0; i < N; i++)
        {
            if (pthread_join(filThreads[i], NULL) != 0)
            {
                perror("Failed to join thread");
            }
        }

        // while(!is_empty(filqueue)){
        //     printf("%s\n",dequeue(&filqueue));
        // }



    }

    if (brokeWidth)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}

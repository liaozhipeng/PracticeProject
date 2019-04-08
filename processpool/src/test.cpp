// #include <stdio.h>
// #include <sys/shm.h>
// #include <sys/ipc.h>
// #include <unistd.h>
// #include<string.h>
// #include <iostream>
// using namespace std;
// static char msg[] = "ASDFGH";

// int main()
// {
//     key_t key;
//     int semid,shmid;
//     char i,*shms,*shmc;
//     int value = 0;
//     char buffer[90];
//     printf("12123123131\n");
//     key = ftok("/ipc/sem",'b');
//     shmid = shmget(20,1024,0);
    
//     pid_t p = fork();
//     if(p>0){
//         shms = (char*)shmat(shmid,0,0);
//         printf("1\n");
//         memcpy(shms,msg,strlen(msg)+1);
//         sleep(5);
//         shmdt(shms);
//     }
//     else if(p==0)
//     {
//         shmc = (char*)shmat(shmid,0,0);
//         sleep(10);
//         printf("2\n");
//         printf("11:%s\n",msg);
//         printf("11:%s\n",shmc);
//         printf("3\n");
//         shmdt(shmc);
//     }
//     while(1);
//     return 0;
    
// }
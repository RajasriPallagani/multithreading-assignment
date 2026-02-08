#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
 
struct message {
    long type;
    char text[100];
};
 
int main()
{
    key_t key;
    int msgid;
    struct message msg;
 
    key = ftok("progfile", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);
 
    msg.type = 1;
    strcpy(msg.text, "Hello from message queue");
 
    msgsnd(msgid, &msg, sizeof(msg.text), 0);
    msgrcv(msgid, &msg, sizeof(msg.text), 1, 0);
 
    printf("%s\n", msg.text);
 
    msgctl(msgid, IPC_RMID, NULL);
 
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define FIRST 1
#define SECOND 2
#define FIRST_WRITE 0
#define SECOND_READ 1
#define SECOND_WRITE 2
#define FIRST_READ 3
#define MESSAGE_SIZE 64
#define NAME_SIZE 15

//ka8olikh metabliti gia na 3eroume pote labame shma
volatile sig_atomic_t gotsig=-1;

//an er8ei shma ana8etoume tin timh tou sig so gotsig
void handler (int sig) {
	
	gotsig=sig;
	
}

//sinartisi pou pernei os orisma an oi xrhstes einai in i' out kai poios paei na fugei apo thn sunomilia
int end(char first_situation[3], char second_situation[3], int who){
	
	//an enas apo tous 2 einai out 8a epistrafei -1 gia na kanoume ka8arizoume ta panta
	if((strcmp(first_situation,"out")==0)||(strcmp(second_situation,"out")==0)) {
		return -1;
	}
	else {
		//an einai o protos 8a allaksoume thn katastash tou se out kai 8a epistrepsoume 1
		if(who==FIRST){
			strcpy(first_situation, "out");
			return 1;
		}
		//an einai o deuteros 8a allaksoume thn katastash tou se out kai 8a epistrepsoume 2
		else {
			strcpy(second_situation, "out");
			return 2;
		}
	}
}


int main(int argc, char *argv[]){
	
	//dhlwsei metablitwn
	key_t key;
	pid_t check_wait;
	int shmid, pid, check, who, semid, who_write, who_read, who_read_in_read, who_write_in_read;
	char *p, first_situation[3], second_situation[3], name[15], *point_write, *point_read;
	struct sembuf op;
	struct sigaction act={{0}};
	
	//an den dw8oun 4 orismata to programma 8a termatisei
	if(argc!=4){
		return 0;
	}
	
	//dhmiourgeia tou monadikoy kleidioy gia tin shmget kai msgget
	key=ftok(".", 'a');
	if(key==-1){
		perror("ftok\n");
		return -1;
	}
	
	//dhmiourgeia koinoxristeis mnimis
	//15 bytes gia to onoma tou protou kai 64 gia to mnm tou 15 bytes gia to onoma tou deuterou 
	//kai 64 gia to mnm touu kai 3 bytes gia to an o protos einai "in" h "out" kai alla 3 bytes gia ton allo
	shmid=shmget(key, 164*sizeof(char), IPC_CREAT|IPC_EXCL|S_IRWXU);
	if(shmid!=-1){
		
		//an petixei kanoume shmat gia na mporoume na diabasoume apo tin koinoxristi mnimi
		p= shmat(shmid, NULL, 0);
		if(p==(char*)(-1)){
			perror("shmat 1\n");
			return -1;
		}
		
		//dhmiourgeia 4 simatoforvn
		semid=semget(key, 4, IPC_CREAT|S_IRWXU);
		if(semid==-1){
			perror("semget\n");
			return -1;
		}
		
		//arxikopoioume tis times ton simatoforon
		check=semctl(semid, FIRST_WRITE, SETVAL, 1);
		if(check==-1){
			perror("semctl\n");
			return -1;
		}
		
		check=semctl(semid, SECOND_READ, SETVAL, 0);
		if(check==-1){
			perror("semctl\n");
			return -1;
		}
		
		check=semctl(semid, SECOND_WRITE, SETVAL, 1);
		if(check==-1){
			perror("semctl\n");
			return -1;
		}
		
		check=semctl(semid, FIRST_READ, SETVAL, 0);
		if(check==-1){
			perror("semctl\n");
			return -1;
		}
		
		who=FIRST;
		
		//eisagoume sthn koinoxrish mnimi ta aparaitita stoixeia
		strcpy(p,argv[1]);
		strcpy(p+NAME_SIZE+MESSAGE_SIZE,argv[3]);
		strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE),"in");
		strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3,"out");
		
		//dhmiourgoume to shma me handler
		act.sa_handler=handler;
		check=sigaction(SIGINT, &act, NULL);
		if(check==-1){
			perror("sigaction\n");
			return -1;
		}
		
		//dhmiourgeia paidiou gia diabazoume kai na grafoume mnm tautoxrona
		pid=fork();
		if(pid==-1){
			perror("fork\n");
			return -1;
		}
		
		
		if(pid==0) {
			while(1){
				
// 				katebazoume ton simatoforo oti mporei na diabasei o xristis
				op.sem_num=FIRST_READ;
				op.sem_op=-1;
				op.sem_flg=0;
				check=semop(semid, &op, 1);
				if(check==-1){
					perror("semop\n");
					return -1;
				}
				
				printf("%s:%s", argv[3],p+(2*NAME_SIZE)+MESSAGE_SIZE);
				
// 				anebazoume ton simatoforo gia na mporei na paei na grapsei o allos xristis
				op.sem_num=SECOND_WRITE;
				op.sem_op=1;
				op.sem_flg=0;
				check=semop(semid, &op, 1);
				if(check==-1){
					perror("semop\n");
					return -1;
				}
				
			}
		}
		
		while(1){
			
// 			katebazoume ton simatoforo oti o xristis mporei na grapsei
			op.sem_num=FIRST_WRITE;
			op.sem_op=-1;
			op.sem_flg=0;
			check=semop(semid, &op, 1);
			if(check==-1){
				perror("semop\n");
				return -1;
			}
			
			//diabazoume to mnm kai to eisagoume sthn koinoxristi
			fgets(p+NAME_SIZE, MESSAGE_SIZE-1, stdin);
			
			//an er8ei to shma kaloume thn end kai an einai kapoios out kanoume break gia na sbisoume ta panta
			//allios grafoume sthn katastash tou out kai termatizoume to programm afou skotosoume to paidi
			if(gotsig!=-1){
				strcpy(first_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE));
				strcpy(second_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3);
				check=end(first_situation,second_situation,who);
				if(check==1){
					strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE),"out");
					
					op.sem_num=FIRST_WRITE;
					op.sem_op=1;
					op.sem_flg=0;
					check=semop(semid, &op, 1);
					if(check==-1){
						perror("semop\n");
						return -1;
					}
					
					kill(pid, SIGTERM);
					check_wait=waitpid(pid, NULL, 0);
					if(check_wait==-1){
						perror("waitpid\n");
						return -1;
					}
					
					shmdt(p);
					if(p==(char*)(-1)){
						perror("shmdt\n");
						return -1;
					}
					
					return 0;
				}
				else 
					break;
			}
			
			//an grapsoume quit kaloume thn end kai an einai kapoios out kanoume break gia na sbisoume ta panta
			//allios grafoume sthn katastash tou out kai termatizoume to programm afou skotosoume to paidi
			if((strcmp(p+NAME_SIZE,"quit\n")==0)||(strcmp(p+NAME_SIZE,"QUIT\n")==0)){
				strcpy(first_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE));
				strcpy(second_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3);
				check=end(first_situation,second_situation,who);
				if(check==1){
					strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE),"out");
					
					op.sem_num=FIRST_WRITE;
					op.sem_op=1;
					op.sem_flg=0;
					check=semop(semid, &op, 1);
					if(check==-1){
						perror("semop\n");
						return -1;
					}
					
					kill(pid, SIGTERM);
					check_wait=waitpid(pid, NULL, 0);
					if(check_wait==-1){
						perror("waitpid\n");
						return -1;
					}
					
					shmdt(p);
					if(p==(char*)(-1)){
						perror("shmdt\n");
						return -1;
					}
					
					return 0;
				}
				else 
					break;
			}
			
// 			anebazoume ton simatoforo oti o allos xristis mporei na diabasei
			op.sem_num=SECOND_READ;
			op.sem_op=1;
			op.sem_flg=0;
			check=semop(semid, &op, 1);
			if(check==-1){
				perror("semop\n");
				return -1;
			}
			
		}
	}
	else if(shmid==-1){
		//an i koinoxristh mnimi uparxei
		if(errno==EEXIST){
			
			shmid=shmget(key, 0, 0);
			if(shmid==-1){
				perror("shmget");
				return 0;
			}
			
			
			p= shmat(shmid, NULL, 0);
			if(p==(char*)(-1)){
				perror("shmat 1\n");
				return -1;
			}
			
			semid=semget(key, 4, 0);
			if(semid==-1){
			perror("semget\n");
				return -1;
			}
			
			if((strcmp(p,argv[3])==0)&&(strcmp(p+MESSAGE_SIZE+15,argv[1])==0)){
				who=SECOND;
				who_write_in_read=FIRST_WRITE;
				who_read_in_read=SECOND_READ;
				who_write=SECOND_WRITE;
				who_read=FIRST_READ;
				point_write=p+(2*NAME_SIZE)+MESSAGE_SIZE;
				point_read=p+NAME_SIZE;
				strcpy(name, argv[3]);
				strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3, "in");
			}
			else if((strcmp(p,argv[1])==0)&&(strcmp(p+MESSAGE_SIZE+15,argv[3])==0)){
				if(strcmp(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE), "in")==0){
					return 0;
				}
				else {
					who=FIRST;
					who_write=FIRST_WRITE;
					who_read=SECOND_READ;
					who_write_in_read=SECOND_WRITE;
					who_read_in_read=FIRST_READ;
					point_write=p+NAME_SIZE;
					point_read=p+(2*NAME_SIZE)+MESSAGE_SIZE;
					strcpy(name, argv[3]);
					strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE), "in");
				}
			}
			else {
				printf("WRONG NAME\n");
				return -1;
			}
			
			//dhmiourgoume to shma me handler
			act.sa_handler=handler;
			check=sigaction(SIGINT, &act, NULL);
			if(check==-1){
				perror("semctl\n");
				return -1;
			}
			
			//dhmiourgeia paidiou gia diabazoume kai na grafoume mnm tautoxrona
			pid=fork();
			if(pid==-1){
				perror("fork\n");
				return -1;
			}
			
			if(pid==0){
				while(1){
					
// 					 
					op.sem_num=who_read_in_read;
					op.sem_op=-1;
					op.sem_flg=0;
					check=semop(semid, &op, 1);
					if(check==-1){
						perror("semop\n");
						return -1;
					}
					
					printf("%s:%s", name, point_read);
					
					op.sem_num=who_write_in_read;
					op.sem_op=1;
					op.sem_flg=0;
					check=semop(semid, &op, 1);
					if(check==-1){
						perror("semop\n");
						return -1;
					}
					
				}
				
			}
			
			while(1){
				
				op.sem_num=who_write;
				op.sem_op=-1;
				op.sem_flg=0;
				check=semop(semid, &op, 1);
				if(check==-1){
					perror("semop\n");
					return -1;
				}
				
				//diabazoume to mnm kai to eisagoume sthn koinoxristi
				fgets(point_write, MESSAGE_SIZE-1, stdin);
				
				//an er8ei to shma kaloume thn end kai an einai kapoios out kanoume break gia na sbisoume ta panta
				//allios grafoume sthn katastash tou out kai termatizoume to programm afou skotosoume to paidi
				if(gotsig!=-1){
					strcpy(first_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE));
					strcpy(second_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3);
					check=end(first_situation,second_situation,who);
					if(check==1){
						strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE),"out");
						
						op.sem_num=who_write;
						op.sem_op=1;
						op.sem_flg=0;
						check=semop(semid, &op, 1);
						if(check==-1){
							perror("semop\n");
							return -1;
						}
						
						kill(pid, SIGTERM);
						check_wait=waitpid(pid, NULL, 0);
						if(check_wait==-1){
							perror("waitpid\n");
							return -1;
						}
						
						shmdt(p);
						if(p==(char*)(-1)){
							perror("shmdt\n");
							return -1;
						}
						
						return 0;
					}
					else if(check==2){
						strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3,"out");
						
						op.sem_num=who_write;
						op.sem_op=1;
						op.sem_flg=0;
						check=semop(semid, &op, 1);
						if(check==-1){
							perror("semop\n");
							return -1;
						}
						
						kill(pid, SIGTERM);
						check_wait=waitpid(pid, NULL, 0);
						if(check_wait==-1){
							perror("waitpid\n");
							return -1;
						}
						
						shmdt(p);
						if(p==(char*)(-1)){
							perror("shmdt\n");
							return -1;
						}
						
						return 0;
					}
					else
						break;
				}
				
				//an grapsoume quit kaloume thn end kai an einai kapoios out kanoume break gia na sbisoume ta panta
				//allios grafoume sthn katastash tou out kai termatizoume to programm afou skotosoume to paidi
				if((strcmp(point_write,"quit\n")==0)||(strcmp(point_write,"QUIT\n")==0)){
					strcpy(first_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE));
					strcpy(second_situation,p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3);
					check=end(first_situation,second_situation,who);
					if(check==1){
						strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE),"out");
						
						op.sem_num=who_write;
						op.sem_op=1;
						op.sem_flg=0;
						check=semop(semid, &op, 1);
						if(check==-1){
							perror("semop\n");
							return -1;
						}
						
						kill(pid, SIGTERM);
						check_wait=waitpid(pid, NULL, 0);
						if(check_wait==-1){
							perror("waitpid\n");
							return -1;
						}
						
						shmdt(p);
						if(p==(char*)(-1)){
							perror("shmdt\n");
							return -1;
						}
						
						return 0;
					}
					else if(check==2){
						strcpy(p+(2*NAME_SIZE)+(2*MESSAGE_SIZE)+3,"out");
						
						op.sem_num=who_write;
						op.sem_op=1;
						op.sem_flg=0;
						check=semop(semid, &op, 1);
						if(check==-1){
							perror("semop\n");
							return -1;
						}
						
						kill(pid, SIGTERM);
						check_wait=waitpid(pid, NULL, 0);
						if(check_wait==-1){
							perror("waitpid\n");
							return -1;
						}
						
						shmdt(p);
						if(p==(char*)(-1)){
							perror("shmdt\n");
							return -1;
						}
						
						return 0;
					}
					else
						break;
				}
				
				op.sem_num=who_read;
				op.sem_op=1;
				op.sem_flg=0;
				check=semop(semid, &op, 1);
				if(check==-1){
					perror("semop\n");
					return -1;
				}
				
			}
		}
		else{
			perror("shmid\n");
			return -1;
		}
	}
	
	//kanoume shmdt
	shmdt(p);
	if(p==(char*)(-1)){
		perror("shmdt\n");
		return -1;
	}
	
	//ka8arizoume tin koinoxristi mnimi
	check=shmctl(shmid,IPC_RMID,NULL);
	if(check==-1){
		perror("shmctl\n");
		return -1;
	}
	
	//ka8arizoume tous simatoforous pou dhmiourgeisame
	check=semctl(semid, 4, IPC_RMID);
	if(check==-1){
		perror("semctl\n");
		return -1;
	}
	
	//skotonoume to paidi kai kanoume waitpid gia na min afisoume zombi
	kill(pid, SIGTERM);
	check_wait=waitpid(pid, NULL, 0);
	if(check_wait==-1){
		perror("waitpid\n");
		return -1;
	}
	
	return 0;
}
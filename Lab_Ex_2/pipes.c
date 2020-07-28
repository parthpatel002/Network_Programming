#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define BUFLEN 4096

void upper_string(char s[]) {
   int c = 0;   
   while (s[c] != '\0') {
      if (s[c] >= 'a' && s[c] <= 'z') 
         s[c] = s[c] - 32;
      c++;
   }
}

int main(){

char * buf = malloc(sizeof(char) * BUFLEN);
memset(buf, 0, sizeof(char) * BUFLEN);
int p[6][2];
int id = 0, pid = 0;

//gets(buf);
/*fgets(buf, BUFLEN, stdin);
int len = strlen(buf);
buf[len-1] = '\0';
len--; */

for(int i = 0; i < 6; i++)
	if(pipe(p[i]) < 0)
		exit(1);

for(int i = 0; i < 5; i++){
	pid = fork();
	if(pid == 0){
		id = i + 1;
		for(int j = 0; j < 6; j++)
			for(int k = 0; k < 2; k++)
				if(((j == id - 1) && (k == 0)) || ((j == id) && (k == 1)))
					continue;
				else
					close(p[j][k]);
				
		break;
	}
	else{
		if(i == 4){
			for(int j = 0; j < 6; j++)
				for(int k = 0; k < 2; k++)
					if(((j == 5) && (k == 0)) || ((j == 0) && (k == 1)))
						continue;
					else
						close(p[j][k]);
		}			
		continue;
	}		
}

switch(id){
	case 0:
		fgets(buf, BUFLEN, stdin);
		int len = strlen(buf);
		buf[len-1] = '\0';
		len--; 
		write(p[0][1], buf, len+1);
		close(p[0][1]);
		len = read(p[5][0], buf, BUFLEN);
		printf("P1 %d %s\n", getpid(), buf);
	break;
	case 1:
		len = read(p[0][0], buf, BUFLEN);
		upper_string(buf);
		printf("C1 %d %s\n", getpid(), buf);
		write(p[1][1], buf, strlen(buf)+1);
		close(p[1][1]);
	break;
	case 2:
		len = read(p[1][0], buf, BUFLEN);
		buf = buf + 1;
		printf("C2 %d %s\n", getpid(), buf);
		write(p[2][1], buf, strlen(buf)+1);
		close(p[2][1]);
	break;
	case 3:	
		len = read(p[2][0], buf, BUFLEN);
		len = strlen(buf);
		buf[len-1] = '\0';
		printf("C3 %d %s\n", getpid(), buf);
		write(p[3][1], buf, strlen(buf)+1);
		close(p[3][1]);
	break;
	case 4:
		len = read(p[3][0], buf, BUFLEN);
		buf = buf + 1;
		printf("C4 %d %s\n", getpid(), buf);
		write(p[4][1], buf, strlen(buf)+1);
		close(p[4][1]);
	break;
	case 5:
		len = read(p[4][0], buf, BUFLEN);
		len = strlen(buf);
		buf[len-1] = '\0';
		printf("C5 %d %s\n", getpid(), buf);
		write(p[5][1], buf, strlen(buf)+1);
		close(p[5][1]);
	break;
}

return 0;
}


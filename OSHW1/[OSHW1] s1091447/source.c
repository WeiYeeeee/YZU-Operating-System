#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>

typedef struct region{
	int child_pid, guess, parent1, parent2, child1, child2, num_parent, num_child, bomb_parent, bomb_child, winner, statue; // 1:child, 2:parent
}REGION;

int main(int argc, char** argv){
	int shm_fd = shm_open("myshare", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (shm_fd < 0){
		printf("shm_open failed!\n");
		exit(0);
	}
	if (ftruncate(shm_fd, sizeof(REGION)) != 0){
		printf("ftruncate failed!\n");
		exit(0);
	}
	REGION* ptr = (REGION*)mmap(0, sizeof(REGION), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED){
		printf("mmap failed!\n");
		exit(0);
	}
	ptr->statue = 1;
	ptr->num_parent=2;
	ptr->num_child=2;
	ptr->bomb_parent=0;
	ptr->bomb_child=0;
	ptr->winner = -1;
	pid_t pid = fork();
	if (pid == 0){
		ptr->child_pid = getpid();
		int seed = atoi(argv[2]);
		int guess_child[16];
		srand(seed);
		while (ptr->statue == 1);
		printf("[%d Child]:  Random Seed %d\n", getpid(), seed);
		ptr->statue = 1;
		while (ptr->statue == 1);
		int gunboat_bow_child = rand() % 16, gunboat_stern_child = rand() % 16;
		while (true){
			if ((gunboat_stern_child == gunboat_bow_child + 4 || gunboat_stern_child == gunboat_bow_child - 4 || gunboat_stern_child == gunboat_bow_child + 1 || gunboat_stern_child == gunboat_bow_child - 1) && gunboat_bow_child >= 0 && gunboat_bow_child <= 15)
				break;
			else
				gunboat_stern_child = rand() % 16;
		}
		printf("[%d Child]: The gunboat: (%d,%d)(%d,%d)\n", getpid(), gunboat_bow_child / 4, gunboat_bow_child % 4, gunboat_stern_child / 4, gunboat_stern_child % 4);
		ptr->child1 = gunboat_bow_child;
		ptr->child2 = gunboat_stern_child;
		ptr->statue = 1;
		while (ptr->statue == 1);
		while (ptr->num_child != 0 && ptr->num_parent != 0){
			if (ptr->guess == ptr->child1 || ptr->guess == ptr->child2){
				ptr->num_child--;
				if (ptr->num_child == 0){
					printf("[%d Child]: hit and sinking\n", getpid());
					ptr->winner = 2;
					ptr->statue = 1;
					while (ptr->statue == 1);
					break;
				}
				else
					printf("[%d Child]: hit\n", getpid());
				if (ptr->guess == ptr->child1)
					ptr->child1 = 16;
				else
					ptr->child2 = 16;
			}
			else
				printf("[%d Child]: missed\n", getpid());
			int gunboat_guess = rand() % 16;
			while (guess_child[gunboat_guess] != 0)
				gunboat_guess = rand() % 16;
			guess_child[gunboat_guess] = 1;
			printf("[%d Child]: bombing (%d,%d)\n", getpid(), gunboat_guess / 4, gunboat_guess % 4);
			ptr->guess = gunboat_guess;
			ptr->bomb_child++;
			ptr->statue = 1;
			while (ptr->statue == 1);
		}
	}
	else if (pid > 0){
		int seed = atoi(argv[1]);
		int guess_parent[16];
		srand(seed);
		printf("[%d Parent]: Random Seed %d\n", getpid(), seed);
		ptr->statue = 2;
		while (ptr->statue == 2);
		int gunboat_bow_parent = rand() % 16, gunboat_stern_parent;
		while (true){
			if ((gunboat_stern_parent ==  gunboat_bow_parent + 4 || gunboat_stern_parent ==  gunboat_bow_parent - 4 || gunboat_stern_parent ==  gunboat_bow_parent + 1 || gunboat_stern_parent ==  gunboat_bow_parent - 1) &&  gunboat_bow_parent >= 0 &&  gunboat_bow_parent <= 15)
				break;
			else
				gunboat_stern_parent = rand() % 16;
		}
		printf("[%d Parent]: The gunboat: (%d,%d)(%d,%d)\n", getpid(),gunboat_bow_parent/4,gunboat_bow_parent%4,gunboat_stern_parent/4,gunboat_stern_parent%4);
		ptr->parent1 = gunboat_bow_parent;
		ptr->parent2 = gunboat_stern_parent;
		ptr->statue = 2;
		while (ptr->statue == 2);
		while (ptr->num_child !=0 && ptr->num_parent !=0){
			int gunboat_guess = rand() % 16;
			while (guess_parent[gunboat_guess] != 0)
				gunboat_guess = rand() % 16;
			guess_parent[gunboat_guess] = 1;
			printf("[%d Parent]: bombing (%d,%d)\n", getpid(), gunboat_guess / 4, gunboat_guess % 4);
			ptr->guess = gunboat_guess;
			ptr->bomb_parent++;
			ptr->statue = 2;
			while (ptr->statue == 2);
			if (ptr->num_child == 0)
				break;
			else{
				if (ptr->guess == ptr->parent1 || ptr->guess == ptr->parent2){
					ptr->num_parent--;
					if (ptr->num_parent == 0){
						printf("[%d Parent]: hit and sinking\n", getpid());
						ptr->winner = 1;
						break;
					}
					else
						printf("[%d Parent]: hit\n", getpid());
					if (ptr->guess == ptr->parent1)
						ptr->parent1 = 16;
					else
						ptr->parent2 = 16;
				}
				else
					printf("[%d Parent]: missed\n", getpid());
			}
		}
		if(ptr->winner==1)
			printf("[%d Parent]: %d wins with %d bombs\n", getpid(), ptr->child_pid, ptr->bomb_child);
		else
			printf("[%d Parent]: %d wins with %d bombs\n", getpid(), getpid(), ptr->bomb_parent);
	}
	else{
		printf("fork error\n");
		exit(0);
	}
	if (munmap(ptr, sizeof(REGION)) != 0){
		printf("munmap error\n");
		exit(0);
	}
	if (shm_unlink("myshare") != 0){
		printf("shm_unlink error");
		exit(0);
	}
}
/*
 * Header file for BFS 
 */

#ifndef BFS_H_
#define BFS_H_

struct queue {
	int items[SIZE]; 
	int front; 
	int rear; 
};

struct node { 
	int vertex; 
	struct node* next;
};

struct Graph {
	int numVertices;
	struct node** adjLists;
	int* visited;
};

struct queue* createQueue();
struct node* createNode(int);

void enqueue(struct queue* q int);
int dequeue(struct queue* q);

void display(struct queue* q);
int isEmpty(struct queue* q); 
void printQueue(struct queue* q); 

#endif

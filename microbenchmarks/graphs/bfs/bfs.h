/*
 * Header file for BFS 
 */

struct queue {
	int items[SIZE]; 
	int front; 
	int rear; 
};

struct queue* createQueue();


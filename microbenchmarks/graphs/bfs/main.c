#include <stdio,h> 
#include <stdlib.h> 

static struct node { 
	int vertex; 
	struct node* next;
}

static struct node* createNode(int v) { 
	struct node* newNode = malloc(sizeof(struct node));
	newNode->vertex = v; 
	newNode->next = NULL; 
	
	return newNode; 
}

static struct Graph* createGraph(int vertices) {
	// some graph with vertices 
}


static struct Edges*(struct Graph* graph, int scr, int dest)
{
	//adding edges 
}	





int main(int argv, char **argc) { 

}

#include <stdio.h>
#include <stdlib.h>
#include "bfs.h"

void bfs(struct Graph* graph, int startVertex) { 
	struct queue* q = createQueue(); 
	
	graph -> visited[startVertex] = 1;
	enqueue(q, startVertex); 
	
	while(!isEmpty(q)) { 
		printQueue(q); 
		int currentVertex = dequeue(q);
		printf("Visited %d\n", currentVertex);
		
		struct node* temp = graph -> adjLists[currentVertex];
	
		while(temp) { 
			int adjVertex = temp -> vertex;
			
			if(graph -> visted[adjVertex] == 0) { 
				
				graph -> visted[adjVertex] = 1; 
				enqueue(q, adjVertex); 
			}

			temp = temp -> next; 
		}
	}
}

struct node* createNode(int v) { 
		
	struct node* newNode = malloc(sizeof(struct node)); 
	newNode -> vertex = v; 
	newNode -> next = NULL; 
	return newNode; 
}




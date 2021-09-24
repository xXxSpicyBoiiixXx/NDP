#include <stdio.h>
#include <stdlib.h>

int sum(int arr[], int n) 
{
	int sum = 0;

	for(int i = 0; i < n; i++) { 
		sum += arr[i];
	}
	return sum;
}

int main() {
	
	int sumArray[10];
	
	int i;
		
	int num1, num2; 

	for(i = 0; i < 10; i++) {
		sumArray[i] = rand(); 
	}

	printf("Sum: %d", sum(sumArray, 10)); 
	
	do{ 
	printf("Enter palce in the array you want to replace followed by the value ");
	scanf("%d %d", &num1, &num2); 
	sumArray[num1] = num2;
	sum(sumArray, 10); 
	printf("New sum: %d", sum(sumArray, 10)); 
	}	
	while(num1 > 0 && num1 < 9); 

return 0;
}


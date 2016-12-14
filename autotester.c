int main(){
	
	int run = 1;
	int i = 0;
	int j = 0;
	int k = 0;
	char buffer[256];
	
	while(run){
		
		printf("Enter a function type:\n");
		scanf("%d", &i);
		printf("You entered: %d\n", i);
		
		switch(i){
			
			case NET_SERVERINIT:
				printf("Enter hostname and connection mode:\n");
				scanf("%s %d", buffer, &j);
				printf("You entered: %s %d\n", buffer, j);
				netserverinit(buffer, j);
				break;
			
			case NET_OPEN:
				printf("Enter file pathname and access flags:\n");
				scanf("%s %d", buffer, &j);
				printf("You entered: %s %d\n", buffer, j);
				netopen(buffer, j);
				break;
				
			case NET_READ:
				printf("Enter file descriptor, buffer, and number of bytes:\n");
				scanf("%d %s %d", &j, buffer, &k);
				printf("You entered: %d %s %d\n", j, buffer, k);
				netread(j, buffer, k);
				break;
				
			case NET_WRITE:
				printf("Enter file descriptor, buffer, and number of bytes:\n");
				scanf("%d %s %d", &j, buffer, &k);
				printf("You entered: %d %s %d\n", j, buffer, k);
				netread(j, buffer, k);
				break;

			case NET_CLOSE:
				printf("Enter file descriptor:\n");
				scanf("%d", &j);
				printf("You entered: %d\n", j);
				netclose(j);
				break;
		}
	}
}










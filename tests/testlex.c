// program de testare a analizorului lexical, v1.1

int main()
{
	int i;
	i=0;
	while(i<10){
		if(i/2==1)puti(i);
		i=i+1;
		}
	if(4.1==49e-1&&0.49E1==2.45+2)puts("yes");
	putc('\'');
	puts("pentru \" se foloseste \\\"");	// pentru \n
	puts("\n\txx");
	putc('\n');
	return 0;
}

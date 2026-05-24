int max(int a,int b){
	if(a>b){
		return a;
	}
	else{
		return b;
	}
}

int max3(int a,int b,int c){
	if(a>b&&a>c){
		return a;
	}
	else if(b>c&&b>a){
		return b;
	}
	else{
		return c;
	}
}

// test 3
int nSolutii(double a,double b,double c){
	double d2;
	d2=b*b-4*a*c;
	int n;
	if(d2<0){n=0;}
	else if(d2==0){n=1;}
	else{n=2;}
	return n;
}

// test 4
int fact(int n){
	if(n<0)return 0;
	int i;
	i=2;
	int r;
	r=1;
	while(i<=n){
		r=r*i;
		i=i+1;
	}
	return r;
}

// test 5
int ack(int m,int n){
	if(m==0)return n+1;
	else if(m>0&&n==0)return ack(m-1,1);
	return ack(m-1,ack(m,n-1));
}

// test 7
int cnt(int vv,int x,int y){
	if(x>y)return -1;
	int n;
	n=0;
	while(x<y){
		n=n+1;
		y=y-x;
	}
	return n;
}

// test 8
int nd(int v){
	int n;
	n=0;
	if(v<0)v=-v;
	while(v>0){
		v=v/10;
		n=n+1;
	}
	return n;
}

// test 9
double f9(double x,double y){
	if(x<y){
		while(x<0)x=x+7;
	}else{
		x=-y;
	}
	return x;
}

// test 10
int g10(int a,int b,double x){
	if(a<b){
		if(a<x&&x<b)return 1;
	}else{
		if(b<x&&x<a)return 1;
	}
	return 0;
}

// test 12
int nc(int a,int b,int c){
	int d1;
	d1=a-b;
	int d2;
	d2=a-c;
	while(d1<d2){
		if(a!=d1)d1=d1/2;
		d2=a-d1;
	}
	return d1;
}

// test 15
int f15(double x,double y){
	int n;
	n=0;
	while(x>y){
		x=x-y;
		n=n+1;
		if(n==100)return 100;
	}
	return n;
}

// test 16
int h16(int a,int b){
	int n;
	n=0;
	double ff;
	ff=0;
	while(a>b){
		ff=a/(double)b;
		if(ff==(int)ff)n=n+1;
		b=b-1;
	}
	return n;
}

// test 17
int f17(int a,int b,int c){
	int n;
	n=0;
	while(max3(a,b,c)<100){
		a=a*2;
		c=a+b;
		n=n+1;
	}
	return n;
}

// test 18
int h18(double x,double y,double z){
	if(x<0){
		if(y==z)return 1;
	}else{
		if(y==z)return 0;
	}
	return 0;
}

// test 19
int f19(int x){
	double r;
	r=x;
	int i;
	i=0;
	while(r>10){
		r=2*(r+0.5);
		i=i+1;
	}
	return i;
}
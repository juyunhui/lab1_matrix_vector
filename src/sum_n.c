#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    double tt() {
        LARGE_INTEGER f,c;
        QueryPerformanceFrequency(&f);
        QueryPerformanceCounter(&c);
        return (double)c.QuadPart/(double)f.QuadPart;
    }
#else
    #include <sys/time.h>
    double tt() {
        struct timeval x;
        gettimeofday(&x,NULL);
        return x.tv_sec + x.tv_usec/1000000.0;
    }
#endif

void init(double* a, int n) {
    for (int i=0;i<n;++i) a[i]=i+1.0;
}

double M1(double* a, int n) {
    double s=0;
    for (int i=0;i<n;++i) s+=a[i];
    return s;
}

double M2(double* a, int n) {
    double x=0,y=0;
    int i=0;
    for(;i+1<n;i+=2){ x+=a[i]; y+=a[i+1]; }
    for(;i<n;++i) x+=a[i];
    return x+y;
}

double M3(double* a, int n) {
    double p=0,q=0,r=0,s=0;
    int i=0;
    for(;i+3<n;i+=4){ p+=a[i]; q+=a[i+1]; r+=a[i+2]; s+=a[i+3]; }
    for(;i<n;++i) p+=a[i];
    return p+q+r+s;
}

double M4(double* a, int n) {
    double z=0;
    int i=0;
    for(;i+7<n;i+=8){
        z+=a[i]+a[i+1]+a[i+2]+a[i+3]+a[i+4]+a[i+5]+a[i+6]+a[i+7];
    }
    for(;i<n;++i) z+=a[i];
    return z;
}

double M5(double* a, int n) {
    double* b = (double*)malloc(n*8);
    if(!b)return 0;
    memcpy(b,a,n*8);
    int sz=n;
    while(sz>1){
        int nsz=sz/2;
        for(int i=0;i<nsz;++i) b[i]=b[i*2]+b[i*2+1];
        sz=nsz;
    }
    double ans=b[0];
    free(b);
    return ans;
}
/// @brief 
/// @param a 
/// @param n 
/// @return 
double M6(double* a, int n) {
    if(n==1)return a[0];
    if(n==0)return 0;
    int h=n/2;
    double* t=(double*)malloc(h*8);
    if(!t)return 0;
    for(int i=0;i<h;++i) t[i]=a[i]+a[n-1-i];
    double ans=M6(t,h);
    free(t);
    return ans;
}
/// @brief 
/// @param name 
/// @param f 
/// @param a 
/// @param n 
/// @param rep 
void bench(char* name, double (*f)(double*,int), double* a, int n, int rep) {
    double t0,t1,tot=0,res=0;
    res=f(a,n);
    for(int i=0;i<rep;++i){
        t0=tt();
        res=f(a,n);
        t1=tt();
        tot+=(t1-t0);
    }
    printf("%-25s | n=%8d | avg=%.6f sec | res=%.0f\n", name, n, tot/rep, res);
}
/// @brief 
/// @return 
int main() {
    int sizes[]={1024,4096,16384,65536,262144,1048576};
    int L=sizeof(sizes)/sizeof(sizes[0]);
    puts("\nSum Benchmark :");
    #ifdef __x86_64__
        puts("x86_64");
    #elif defined(__aarch64__)
        puts("ARM64");
    #endif
    for(int k=0;k<L;++k){
        int n=sizes[k];
        double* a=(double*)malloc(n*8);
        if(!a){printf("OOM %d\n",n);continue;}
        init(a,n);
        int rep=100;
        if(n>=65536) rep=10;
        if(n>=262144) rep=5;
        if(n>=1048576) rep=2;
        printf("--- n=%d (rep=%d) ---\n", n, rep);
        bench("Naive", M1, a, n, rep);
        bench("2-way", M2, a, n, rep);
        bench("4-way", M3, a, n, rep);
        bench("Unroll8", M4, a, n, rep);
        bench("Pairwise", M5, a, n, rep);
        if(n<=65536) bench("Recursive", M6, a, n, rep);
        putchar(10);
        free(a);
    }
    puts("\nFloat Precision:");
    int T=10000;
    double* b=(double*)malloc(T*8);
    for(int i=0;i<T;++i) b[i]=1e10+(i%1000)*1e-6;
    double r1=M1(b,T), r2=M2(b,T), r3=M5(b,T);
    printf("Naive:     %.15f\n", r1);
    printf("2-way:     %.15f\n", r2);
    printf("Pairwise:  %.15f\n", r3);
    printf("Diff1:     %.15e\n", r1-r2);
    printf("Diff2:     %.15e\n", r1-r3);
    free(b);
    return 0;
}
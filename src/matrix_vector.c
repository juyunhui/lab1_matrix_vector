/**
 
 * 实验一：矩阵与向量内积计算 - Cache性能分析程序
 * 功能：对比逐列访问(平凡算法)与逐行访问(Cache优化)的性能差异
 * 编译：gcc -o matrix_vector matrix_vector.c -lm -O2

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    double shi_jian() {
        LARGE_INTEGER a, b;
        QueryPerformanceFrequency(&a);
        QueryPerformanceCounter(&b);
        return (double)b.QuadPart / (double)a.QuadPart;
    }
#else
    #include <sys/time.h>
    double shi_jian() {
        struct timeval x;
        gettimeofday(&x, NULL);
        return x.tv_sec + x.tv_usec / 1000000.0;
    }
#endif

typedef struct {
    double* p;
    double* v;
    double* r;
    int d;
} CanShu;
//
void tian(double* a, double* b, int c) {
    for (int i = 0; i < c; ++i) {
        b[i] = i + 1.0;
        for (int j = 0; j < c; ++j) {
            a[i * c + j] = (double)(i + j);
        }
    }
}
//
int dui(double* x, double* y, int n, double e) {
    for (int i = 0; i < n; ++i) {
        if (fabs(x[i] - y[i]) > e) {
            printf("cuowu %d: %f vs %f\n", i, x[i], y[i]);
            return 0;
        }
    }
    return 1;
}
/// @param A 
/// @param B 
/// @param C 
/// @param N 
void fangfa1(double* A, double* B, double* C, int N) {
    for (int col = 0; col < N; ++col) {
        C[col] = 0.0;
        for (int row = 0; row < N; ++row) {
            C[col] += A[row * N + col] * B[row];
        }
    }
}

void fangfa2(double* A, double* B, double* C, int N) {
    for (int i = 0; i < N; ++i) C[i] = 0.0;
    for (int r = 0; r < N; ++r) {
        double tmp = B[r];
        for (int c = 0; c < N; ++c) {
            C[c] += A[r * N + c] * tmp;
        }
    }
}

void fangfa3(double* A, double* B, double* C, int N) {
    for (int i = 0; i < N; ++i) C[i] = 0.0;
    for (int r = 0; r < N; ++r) {
        double tmp = B[r];
        int c = 0;
        while (c + 3 < N) {
            C[c]   += A[r * N + c]   * tmp;
            C[c+1] += A[r * N + c+1] * tmp;
            C[c+2] += A[r * N + c+2] * tmp;
            C[c+3] += A[r * N + c+3] * tmp;
            c += 4;
        }
        while (c < N) {
            C[c] += A[r * N + c] * tmp;
            c++;
        }
    }
}
/// @brief 
/// @param fn 
/// @param A 
/// @param B 
/// @param C 
/// @param N 
/// @param R 
/// @param name 
void ce(void (*fn)(double*,double*,double*,int), double* A, double* B, double* C, int N, int R, char* name) {
    double t0, t1, tt = 0.0;
    fn(A, B, C, N);
    for (int i = 0; i < R; ++i) {
        t0 = shi_jian();
        fn(A, B, C, N);
        t1 = shi_jian();
        tt += (t1 - t0);
    }
    printf("%-28s | N=%5d | avg=%.6f sec | tot=%.3f sec (R=%d)\n", name, N, tt/R, tt, R);
}

int main() {
    int S[] = {64,128,256,512,1024,2048};
    int L = sizeof(S)/sizeof(S[0]);
    printf("\nMatrix-Vector:\n");
    #ifdef __x86_64__
        puts("x86_64");
    #elif defined(__aarch64__)
        puts("ARM64");
    #endif
    for (int k = 0; k < L; ++k) {
        int N = S[k];
        double* A = (double*)malloc(N*N*8);
        double* B = (double*)malloc(N*8);
        double* C1 = (double*)malloc(N*8);
        double* C2 = (double*)malloc(N*8);
        double* C3 = (double*)malloc(N*8);
        if (!A||!B||!C1||!C2||!C3) { printf("OOM %d\n",N); exit(1); }
        tian(A, B, N);
        int R = 100;
        if (N>=1024) R=10;
        if (N>=2048) R=5;
        ce(fangfa1, A,B,C1,N,R,"Naive");
        ce(fangfa2, A,B,C2,N,R,"CacheOpt");
        ce(fangfa3, A,B,C3,N,R,"Unroll8");
        if (dui(C1,C2,N,1e-6)) puts(" [OK] naive vs cache");
        if (dui(C1,C3,N,1e-6)) puts(" [OK] naive vs unroll");
        putchar(10);
        free(A); free(B); free(C1); free(C2); free(C3);
    }
    return 0;
}
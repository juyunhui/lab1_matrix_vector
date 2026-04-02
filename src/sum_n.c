#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    double get_time() {
        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        return (double)count.QuadPart / (double)freq.QuadPart;
    }
#else
    #include <sys/time.h>
    double get_time() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }
#endif

void init_array(double* arr, int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = i + 1.0;
    }
}

double naive_sum(double* arr, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

double two_way_sum(double* arr, int n) {
    double sum1 = 0.0, sum2 = 0.0;
    int i = 0;
    for (; i <= n - 2; i += 2) {
        sum1 += arr[i];
        sum2 += arr[i + 1];
    }
    for (; i < n; i++) {
        sum1 += arr[i];
    }
    return sum1 + sum2;
}

double four_way_sum(double* arr, int n) {
    double sum1 = 0.0, sum2 = 0.0, sum3 = 0.0, sum4 = 0.0;
    int i = 0;
    for (; i <= n - 4; i += 4) {
        sum1 += arr[i];
        sum2 += arr[i + 1];
        sum3 += arr[i + 2];
        sum4 += arr[i + 3];
    }
    for (; i < n; i++) {
        sum1 += arr[i];
    }
    return sum1 + sum2 + sum3 + sum4;
}

double unrolled_sum(double* arr, int n) {
    double sum = 0.0;
    int i = 0;
    for (; i <= n - 8; i += 8) {
        sum += arr[i] + arr[i+1] + arr[i+2] + arr[i+3] +
               arr[i+4] + arr[i+5] + arr[i+6] + arr[i+7];
    }
    for (; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

double pairwise_sum(double* arr, int n) {
    double* temp = (double*)malloc(n * sizeof(double));
    if (!temp) return 0.0;
    memcpy(temp, arr, n * sizeof(double));
    int size = n;
    while (size > 1) {
        int new_size = size / 2;
        for (int i = 0; i < new_size; i++) {
            temp[i] = temp[2 * i] + temp[2 * i + 1];
        }
        size = new_size;
    }
    double result = temp[0];
    free(temp);
    return result;
}

double recursive_sum(double* arr, int n) {
    if (n == 1) return arr[0];
    if (n == 0) return 0.0;
    int half = n / 2;
    double* temp = (double*)malloc(half * sizeof(double));
    if (!temp) return 0.0;
    for (int i = 0; i < half; i++) {
        temp[i] = arr[i] + arr[n - 1 - i];
    }
    double result = recursive_sum(temp, half);
    free(temp);
    return result;
}

void benchmark_sum(const char* name, double (*func)(double*, int), 
                   double* arr, int n, int repeat) {
    double start, end;
    double total_time = 0.0;
    double result = 0.0;
    result = func(arr, n);
    for (int r = 0; r < repeat; r++) {
        start = get_time();
        result = func(arr, n);
        end = get_time();
        total_time += (end - start);
    }
    double avg_time = total_time / repeat;
    printf("%-25s | n=%8d | avg_time=%.6f sec | result=%f\n", 
           name, n, avg_time, result);
}

int main() {
    int sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int repeat_base = 100;
    printf("\nSum of N Numbers Benchmark：\n");
    #ifdef __aarch64__
        printf("Platform: ARM64\n");
    #elif defined(__x86_64__)
        printf("Platform: x86_64\n");
    #else
        printf("Platform: Unknown\n");
    #endif
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        double* arr = (double*)malloc(n * sizeof(double));
        if (!arr) {
            printf("Memory allocation failed for n=%d\n", n);
            continue;
        }
        init_array(arr, n);
        int repeat = repeat_base;
        if (n >= 65536) repeat = 10;
        if (n >= 262144) repeat = 5;
        if (n >= 1048576) repeat = 2;
        printf("--- n = %d (repeat=%d) ---\n", n, repeat);
        benchmark_sum("Naive (chain)", naive_sum, arr, n, repeat);
        benchmark_sum("2-way parallel", two_way_sum, arr, n, repeat);
        benchmark_sum("4-way parallel", four_way_sum, arr, n, repeat);
        benchmark_sum("Unrolled (8x)", unrolled_sum, arr, n, repeat);
        benchmark_sum("Pairwise sum", pairwise_sum, arr, n, repeat);
        if (n <= 65536) {
            benchmark_sum("Recursive sum", recursive_sum, arr, n, repeat);
        }
        printf("\n");
        free(arr);
    }
    printf("\nFloating Point Precision Test:\n");
    int test_n = 10000;
    double* test_arr = (double*)malloc(test_n * sizeof(double));
    for (int i = 0; i < test_n; i++) {
        test_arr[i] = 1e10 + (i % 1000) * 1e-6;
    }
    double sum_naive = naive_sum(test_arr, test_n);
    double sum_2way = two_way_sum(test_arr, test_n);
    double sum_pairwise = pairwise_sum(test_arr, test_n);
    printf("Naive sum:     %.15f\n", sum_naive);
    printf("2-way sum:     %.15f\n", sum_2way);
    printf("Pairwise sum:  %.15f\n", sum_pairwise);
    printf("Difference (naive - 2way): %.15e\n", sum_naive - sum_2way);
    printf("Difference (naive - pairwise): %.15e\n", sum_naive - sum_pairwise);
    free(test_arr);
    return 0;
}
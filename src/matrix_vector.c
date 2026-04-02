#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

// 获取CPU缓存大小（用于显示）
void print_cache_info() {
    printf("\n CPU Cache Information :\n");
    #ifdef __linux__
        system("lscpu | grep -E 'L1d|L1i|L2|L3'");
    #elif defined(_WIN32)
        printf("(Run 'wmic cpu get L2CacheSize,L3CacheSize' in cmd to see cache sizes)\n");
    #else
        printf("(Check system information for cache sizes)\n");
    #endif
    printf("\n\n");
}

// 初始化矩阵和向量
void init_data(double* matrix, double* vector, int n) {
    for (int i = 0; i < n; i++) {
        vector[i] = i + 1.0;
        for (int j = 0; j < n; j++) {
            matrix[i * n + j] = (i + j) * 1.0;
        }
    }
}

// 验证结果
int verify(double* result1, double* result2, int n, double epsilon) {
    for (int i = 0; i < n; i++) {
        if (fabs(result1[i] - result2[i]) > epsilon) {
            printf("Mismatch at index %d: %f vs %f\n", i, result1[i], result2[i]);
            return 0;
        }
    }
    return 1;
}



// 平凡算法：逐列访问（空间局部性差）
void naive_matrix_vector(double* matrix, double* vector, double* result, int n) {
    for (int i = 0; i < n; i++) {
        result[i] = 0.0;
        for (int j = 0; j < n; j++) {
            // 逐列访问：matrix[j * n + i]
            // 每次内层循环访问的内存地址不连续（步长为n）
            result[i] += matrix[j * n + i] * vector[j];
        }
    }
}

// Cache优化算法：逐行访问（空间局部性好）
void cache_optimized_matrix_vector(double* matrix, double* vector, double* result, int n) {
    // 先初始化结果数组
    for (int i = 0; i < n; i++) {
        result[i] = 0.0;
    }
    // 逐行访问：连续内存访问
    for (int j = 0; j < n; j++) {
        double vj = vector[j];
        for (int i = 0; i < n; i++) {
            result[i] += matrix[j * n + i] * vj;
        }
    }
}

// 带循环展开的Cache优化（减少循环开销）
void cache_optimized_unrolled4(double* matrix, double* vector, double* result, int n) {
    for (int i = 0; i < n; i++) {
        result[i] = 0.0;
    }
    
    for (int j = 0; j < n; j++) {
        double vj = vector[j];
        int i = 0;
        // 4路循环展开
        for (; i <= n - 4; i += 4) {
            result[i]     += matrix[j * n + i]     * vj;
            result[i + 1] += matrix[j * n + i + 1] * vj;
            result[i + 2] += matrix[j * n + i + 2] * vj;
            result[i + 3] += matrix[j * n + i + 3] * vj;
        }
        // 处理剩余元素
        for (; i < n; i++) {
            result[i] += matrix[j * n + i] * vj;
        }
    }
}

// 8路循环展开
void cache_optimized_unrolled8(double* matrix, double* vector, double* result, int n) {
    for (int i = 0; i < n; i++) {
        result[i] = 0.0;
    }
    
    for (int j = 0; j < n; j++) {
        double vj = vector[j];
        int i = 0;
        // 8路循环展开
        for (; i <= n - 8; i += 8) {
            result[i]     += matrix[j * n + i]     * vj;
            result[i + 1] += matrix[j * n + i + 1] * vj;
            result[i + 2] += matrix[j * n + i + 2] * vj;
            result[i + 3] += matrix[j * n + i + 3] * vj;
            result[i + 4] += matrix[j * n + i + 4] * vj;
            result[i + 5] += matrix[j * n + i + 5] * vj;
            result[i + 6] += matrix[j * n + i + 6] * vj;
            result[i + 7] += matrix[j * n + i + 7] * vj;
        }
        for (; i < n; i++) {
            result[i] += matrix[j * n + i] * vj;
        }
    }
}

// 分块算法（Blocking）- 进一步优化Cache利用
void blocking_matrix_vector(double* matrix, double* vector, double* result, int n) {
    // 初始化结果
    for (int i = 0; i < n; i++) {
        result[i] = 0.0;
    }
    
    // 分块大小：根据L1 Cache大小调整（这里用64）
    int BLOCK_SIZE = 64;
    
    for (int jj = 0; jj < n; jj += BLOCK_SIZE) {
        int j_end = (jj + BLOCK_SIZE < n) ? jj + BLOCK_SIZE : n;
        for (int i = 0; i < n; i++) {
            double sum = 0.0;
            for (int j = jj; j < j_end; j++) {
                sum += matrix[j * n + i] * vector[j];
            }
            result[i] += sum;
        }
    }
}


void benchmark(const char* name, void (*func)(double*, double*, double*, int), 
               double* matrix, double* vector, double* result, int n, int repeat, int warmup) {
    double start, end;
    double total_time = 0.0;
    
    // 预热
    for (int w = 0; w < warmup; w++) {
        func(matrix, vector, result, n);
    }
    
    // 正式测试
    for (int r = 0; r < repeat; r++) {
        start = get_time();
        func(matrix, vector, result, n);
        end = get_time();
        total_time += (end - start);
    }
    
    double avg_time = total_time / repeat;
    
    // 计算矩阵大小（MB）
    double matrix_mb = (double)(n * n * sizeof(double)) / (1024 * 1024);
    double vector_mb = (double)(n * sizeof(double)) / (1024 * 1024);
    
    printf("%-28s | n=%5d | %8.2f MB | avg=%.6f sec | ratio=%.2fx | repeat=%d\n", 
           name, n, matrix_mb + vector_mb, avg_time, 
           avg_time > 0 ? 1.0 : 1.0, repeat);
}

// 计算加速比并打印对比
void print_comparison(double naive_time, double cache_time, double unrolled_time, double blocking_time) {
    printf("\n  Speedup (Cache vs Naive):   %.2fx\n", naive_time / cache_time);
    printf("  Speedup (Unrolled vs Naive): %.2fx\n", naive_time / unrolled_time);
    if (blocking_time > 0) {
        printf("  Speedup (Blocking vs Naive): %.2fx\n", naive_time / blocking_time);
    }
}


int main() {
    // 测试规模：覆盖L1、L2、L3 Cache大小
    // 假设：L1~32KB, L2~256KB, L3~8MB
    // 矩阵大小 = n*n*8字节
    // n=64: 64*64*8=32KB (L1)
    // n=128: 128*128*8=128KB (L1边界)
    // n=256: 256*256*8=512KB (L2边界)
    // n=512: 512*512*8=2MB (L2/L3之间)
    // n=1024: 1024*1024*8=8MB (L3边界)
    // n=2048: 2048*2048*8=32MB (超过L3)
    // n=4096: 4096*4096*8=128MB (远超Cache)
    
    int sizes[] = {32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    int repeat_base = 100;      // 基础重复次数
    int warmup = 3;             // 预热次数
    
    // 打印系统信息
    printf("\n");
    
    printf("           Matrix-Vector Dot Product - Cache Performance Analysis          \n");
    
    
    #ifdef __aarch64__
        printf("\n  Platform: ARM64\n");
    #elif defined(__x86_64__)
        printf("\n  Platform: x86_64\n");
    #else
        printf("\n  Platform: Unknown\n");
    #endif
    
    print_cache_info();
    
    printf("  Expected Cache Behavior:\n");
    printf("  - L1 Cache (32KB):   n ≤ 64\n");
    printf("  - L2 Cache (256KB):  n ≤ 181\n");
    printf("  - L3 Cache (8MB):    n ≤ 1024\n");
    printf("  - Beyond Cache:      n > 1024\n");
    printf("\n");
    
    printf(" Algorithm                      Size  Memory  Time(avg)  Ratio  Repeat \n");
    
    
    double naive_time, cache_time, unrolled_time, blocking_time;
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        
        // 根据规模调整重复次数
        int repeat = repeat_base;
        if (n >= 512) repeat = 20;
        if (n >= 1024) repeat = 10;
        if (n >= 2048) repeat = 5;
        if (n >= 3072) repeat = 3;
        if (n >= 4096) repeat = 2;
        
        // 分配内存
        double* matrix = (double*)malloc(n * n * sizeof(double));
        double* vector = (double*)malloc(n * sizeof(double));
        double* result_naive = (double*)malloc(n * sizeof(double));
        double* result_cache = (double*)malloc(n * sizeof(double));
        double* result_unrolled = (double*)malloc(n * sizeof(double));
        double* result_blocking = (double*)malloc(n * sizeof(double));
        
        if (!matrix || !vector || !result_naive || !result_cache || 
            !result_unrolled || !result_blocking) {
            printf("  Memory allocation failed for n=%d\n", n);
            break;
        }
        
        // 初始化数据
        init_data(matrix, vector, n);
        
        // 运行benchmark
       
        // 平凡算法
        benchmark("Naive (column-major)", naive_matrix_vector, 
                  matrix, vector, result_naive, n, repeat, warmup);
        naive_time = 0; // 会在benchmark中计算，但我们需要存储
        
        // 重新运行以获取时间（benchmark不返回时间，需要单独存储）
        // 简单方式：单独计时
        double t_start, t_end;
        
        // 平凡算法计时
        for (int w = 0; w < warmup; w++) naive_matrix_vector(matrix, vector, result_naive, n);
        t_start = get_time();
        for (int r = 0; r < repeat; r++) naive_matrix_vector(matrix, vector, result_naive, n);
        t_end = get_time();
        naive_time = (t_end - t_start) / repeat;
        
        // Cache优化计时
        for (int w = 0; w < warmup; w++) cache_optimized_matrix_vector(matrix, vector, result_cache, n);
        t_start = get_time();
        for (int r = 0; r < repeat; r++) cache_optimized_matrix_vector(matrix, vector, result_cache, n);
        t_end = get_time();
        cache_time = (t_end - t_start) / repeat;
        
        // 循环展开计时
        for (int w = 0; w < warmup; w++) cache_optimized_unrolled8(matrix, vector, result_unrolled, n);
        t_start = get_time();
        for (int r = 0; r < repeat; r++) cache_optimized_unrolled8(matrix, vector, result_unrolled, n);
        t_end = get_time();
        unrolled_time = (t_end - t_start) / repeat;
        
        // 分块算法计时
        for (int w = 0; w < warmup; w++) blocking_matrix_vector(matrix, vector, result_blocking, n);
        t_start = get_time();
        for (int r = 0; r < repeat; r++) blocking_matrix_vector(matrix, vector, result_blocking, n);
        t_end = get_time();
        blocking_time = (t_end - t_start) / repeat;
        
        double matrix_mb = (double)(n * n * sizeof(double)) / (1024 * 1024);
        
        printf("  Naive (column-major)       %5d   %7.2f MB   %.6f sec   1.00x   repeat=%d\n", 
               n, matrix_mb, naive_time, repeat);
        printf("  Cache optimized (row-major)  %5d   %7.2f MB   %.6f sec   %.2fx   repeat=%d\n", 
               n, matrix_mb, cache_time, naive_time / cache_time, repeat);
        printf("  Unrolled (8x)              %5d   %7.2f MB   %.6f sec   %.2fx   repeat=%d\n", 
               n, matrix_mb, unrolled_time, naive_time / unrolled_time, repeat);
        printf("  Blocking                    %5d   %7.2f MB   %.6f sec   %.2fx   repeat=%d\n", 
               n, matrix_mb, blocking_time, naive_time / blocking_time, repeat);
        
        // 验证结果
        if (verify(result_naive, result_cache, n, 1e-6)) {
            printf("Results verified\n");
        }
        
        // 释放内存
        free(matrix);
        free(vector);
        free(result_naive);
        free(result_cache);
        free(result_unrolled);
        free(result_blocking);
    }
    

    printf("\n Analysis Summary \n");
    printf("Expected observations:\n");
    printf("1. When n <= 64 (matrix fits in L1 cache): all algorithms perform similarly\n");
    printf("2. When 64 < n <= 256 (L1/L2 boundary): cache optimization shows advantage\n");
    printf("3. When n > 1024 (exceeds L3): naive algorithm degrades significantly\n");
    printf("4. Blocking algorithm may perform best for large matrices\n");
    printf("5. Loop unrolling provides modest improvement by reducing branch overhead\n");
    
    return 0;
}
/*  багатопотокова обробка даних (паралельне множення матриць з pthreads) */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define N 1024
#define NUM_THREADS 4

static double A[N][N], B[N][N], C[N][N];
static long long global_operations_counter = 0;

static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    int start_row;
    int end_row;
} ThreadArgs;

void *thread_multiply(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    long long local_ops = 0;

    /* Кожен потік обробляє свій діапазон рядків матриці C */
    for (int i = args->start_row; i < args->end_row; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += A[i][k] * B[k][j];
                local_ops++;
            }
            C[i][j] = sum;
        }
    }

    pthread_mutex_lock(&counter_mutex);
    global_operations_counter += local_ops;
    pthread_mutex_unlock(&counter_mutex);

    pthread_exit(NULL);
}

void fill_matrix(double mat[N][N])
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            mat[i][j] = (double)(rand() % 100) / 10.0;
}

int main(void)
{
    srand((unsigned)time(NULL));
    printf("Ініціалізація матриць розміром %dx%d...\n", N, N);
    fill_matrix(A);
    fill_matrix(B);

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];

    int rows_per_thread = N / NUM_THREADS;

    printf("Запуск %d потоків POSIX Threads...\n", NUM_THREADS);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Створення потоків */
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].start_row = i * rows_per_thread;
        args[i].end_row = (i == NUM_THREADS - 1) ? N : (i + 1) * rows_per_thread;

        int rc = pthread_create(&threads[i], NULL, thread_multiply, &args[i]);
        if (rc) {
            fprintf(stderr, "Помилка створення потоку %d, код помилки: %d\n", i, rc);
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    pthread_mutex_destroy(&counter_mutex);

    printf("Паралельне обчислення завершено успішно.\n");
    printf("Виконано елементарних операцій: %lld\n", global_operations_counter);
    printf("Час виконання у %d потоків: %.3f с\n", NUM_THREADS, elapsed);

    return 0;
}

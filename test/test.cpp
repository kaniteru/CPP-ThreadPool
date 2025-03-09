#include <kani/thread_pool.hpp>
#include <chrono>
#include <vector>
#include <future>
#include <functional>
#include <iostream>
#include <random>

class Timer {
public:
    void reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]]
    double elapsed() const {
        const auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end_time - m_start).count();
    }

public:
    Timer() : m_start(std::chrono::high_resolution_clock::now()) {}
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

using namespace kani;

// Test: CPU-Intensive Fibonacci Calculation (High Input)
void test_fibonacci_hard(ThreadPool* pool, uint32_t task_count) {
    Timer timer;

    std::vector<std::future<int>> futures;

    // Recursive Fibonacci function using std::function (C++11 compatible)
    std::function<int(int)> fibonacci = [&](int n) -> int {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    };

    for (uint32_t i = 0; i < task_count; i++) {
        auto future = pool->enqueue([fibonacci]() {
            return fibonacci(30);  // Compute the 30th Fibonacci number (harder than 20)
        });
        futures.push_back(std::move(future));
    }

    for (auto& future : futures) {
        future.get(); // Wait for all tasks to complete
    }

    std::cout << "Processed " << task_count << " hard Fibonacci tasks in "
              << timer.elapsed() << " seconds" << std::endl;
}

// Test: Large Prime Number Search
void test_large_prime_search(ThreadPool* pool, uint32_t task_count) {
    Timer timer;

    std::vector<std::future<bool>> futures;

    // Prime-checking function
    auto is_prime = [](int num) -> bool {
        if (num <= 1) return false;
        for (int i = 2; i * i <= num; i++) {
            if (num % i == 0) return false;
        }
        return true;
    };

    // Generate large random numbers for prime checking
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1000000, 2000000);

    for (uint32_t i = 0; i < task_count; i++) {
        int random_number = dist(gen);
        auto future = pool->enqueue([random_number, is_prime]() {
            return is_prime(random_number);
        });
        futures.push_back(std::move(future));
    }

    for (auto& future : futures) {
        future.get(); // Wait for all tasks to complete
    }

    std::cout << "Processed " << task_count << " large prime-checking tasks in "
              << timer.elapsed() << " seconds" << std::endl;
}

// Test: Large Matrix Multiplication
void test_large_matrix_multiplication(ThreadPool* pool, uint32_t task_count) {
    Timer timer;

    std::vector<std::future<void>> futures;

    for (uint32_t i = 0; i < task_count; i++) {
        auto future = pool->enqueue([]() {
            constexpr int size = 300; // Larger matrices (300x300 for higher memory and CPU strain)
            std::vector<std::vector<int>> matrix_a(size, std::vector<int>(size, 1));
            std::vector<std::vector<int>> matrix_b(size, std::vector<int>(size, 2));
            std::vector<std::vector<int>> result(size, std::vector<int>(size, 0));

            // Matrix multiplication (CPU-heavy)
            for (int row = 0; row < size; row++) {
                for (int col = 0; col < size; col++) {
                    for (int k = 0; k < size; k++) {
                        result[row][col] += matrix_a[row][k] * matrix_b[k][col];
                    }
                }
            }
        });
        futures.push_back(std::move(future));
    }

    for (auto& future : futures) {
        future.get(); // Wait for all tasks to complete
    }

    std::cout << "Processed " << task_count << " large matrix multiplication tasks in "
              << timer.elapsed() << " seconds" << std::endl;
}

// Test: Solve System of Linear Equations (Gaussian Elimination)
void test_gaussian_elimination(ThreadPool* pool, uint32_t task_count) {
    Timer timer;

    std::vector<std::future<void>> futures;

    for (uint32_t i = 0; i < task_count; i++) {
        auto future = pool->enqueue([]() {
            constexpr int size = 200; // Size of the system
            std::vector<std::vector<double>> matrix(size, std::vector<double>(size + 1, 1.0));
            std::vector<double> result(size, 0.0);

            // Simulate filling the augmented matrix
            for (int row = 0; row < size; ++row) {
                for (int col = 0; col < size + 1; ++col) {
                    matrix[row][col] = static_cast<double>((row + col + 1) % 100 + 1);
                }
            }

            // Perform Gaussian elimination to solve the system
            for (int i = 0; i < size; i++) {
                for (int j = i + 1; j < size; j++) {
                    double factor = matrix[j][i] / matrix[i][i];
                    for (int k = i; k <= size; k++) {
                        matrix[j][k] -= factor * matrix[i][k];
                    }
                }
            }

            // Back substitution
            for (int i = size - 1; i >= 0; i--) {
                result[i] = matrix[i][size];
                for (int j = i + 1; j < size; j++) {
                    result[i] -= matrix[i][j] * result[j];
                }
                result[i] /= matrix[i][i];
            }
        });
        futures.push_back(std::move(future));
    }

    for (auto& future : futures) {
        future.get(); // Wait for all tasks to complete
    }

    std::cout << "Processed " << task_count << " Gaussian elimination tasks in "
              << timer.elapsed() << " seconds" << std::endl;
}

int main() {
    ThreadPool pool;
    pool.start();

    constexpr uint32_t task_count = 200; // Adjust task count based on system capacity

    // Run hard tests
    test_fibonacci_hard(&pool, task_count);
    test_large_prime_search(&pool, task_count);
    test_large_matrix_multiplication(&pool, task_count);
    test_gaussian_elimination(&pool, task_count);

    pool.stop();
    return 0;
}
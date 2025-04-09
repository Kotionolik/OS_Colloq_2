#pragma once
#include"Singleton.h"
#include<iostream>
#include<vector>

class ArrayData {
public:
    std::vector<double> arr;
    int n = 0;
    int k = 0;
    int active_threads = 0;

    std::mutex mutex;
    std::condition_variable cv_start;
    std::condition_variable cv_all_done;
    std::condition_variable cv_decision;

    bool start = false;
    int done_count = 0;
    std::vector<bool> terminate_flags;

    void Initialize(int n_val, int k_val) {
        n = n_val;
        k = k_val;
        active_threads = k;
        arr.resize(n, 0.0);
        terminate_flags.resize(k, false);
        for (int i = 0; i < n; i++) {
            arr[i] = i + 1;
        }
    }
};

using SingletonArray = Singleton< ArrayData, CreateUsingNew, DefaultLifetime, ClassLevelLockable >;

void Average_setter(int id) {
    ArrayData& data = SingletonArray::Instance();

    {
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cv_start.wait(lock, [&data] { return data.start; });
    }

    while (true) {
        double sum = 0.0;
        int index = id % data.n;
        {
            std::unique_lock<std::mutex> lock(data.mutex);
            for (double val : data.arr) {
                sum += val;
            }
            double avg = sum / data.arr.size();
            data.arr[index] = avg;
        }

        {
            std::unique_lock<std::mutex> lock(data.mutex);
            data.done_count++;

            std::cout << "Thread " << id << " terminated. Resulting array:\n";
            for (double val : data.arr) {
                std::cout << val << ' ';
            }
            std::cout << "\n\n";

            if (data.done_count == data.active_threads) {
                data.cv_all_done.notify_one();
            }
        }

        {
            std::unique_lock<std::mutex> lock(data.mutex);
            data.cv_decision.wait(lock);

            if (data.terminate_flags[id]) {
                break;
            }
        }
    }
}

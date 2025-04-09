#include"ArrayData.h"

int main()
{
    int n, k;
    std::cout << "Enter array size (n): ";
    std::cin >> n;
    while (n < 1)
    {
        std::cout << "Error: array size should be positive. Enter again: ";
        std::cin >> n;
    }
    std::cout << "Enter number of threads (k): ";
    std::cin >> k;
    while (k < 0)
    {
        std::cout << "Error: thread number should be non-negative. Enter again: ";
        std::cin >> k;
    }

    ArrayData& data = SingletonArray::Instance();
    data.Initialize(n, k);

    std::cout << "Initial array: ";
    for (double val : data.arr) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";

    std::vector<std::thread> threads;
    for (int i = 0; i < k; i++) {
        threads.emplace_back(Average_setter, i);
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.start = true;
        data.cv_start.notify_all();
    }

    while (data.active_threads > 0) {

        {
            std::unique_lock<std::mutex> lock(data.mutex);
            data.cv_all_done.wait(lock, [&data] {
                return data.done_count == data.active_threads;
                });
        }

        {
            std::lock_guard<std::mutex> lock(data.mutex);
            std::cout << "Current array: ";
            for (double val : data.arr) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }

        int id_term;
        std::cout << "Enter thread ID to terminate (0-" << k - 1 << "): ";
        std::cin >> id_term;
        if (id_term < 0 || id_term >= k || data.terminate_flags[id_term] == true)
        {
            std::cout << "Error: the thread id is either out of bounds or the thread with this id was already completed. Enter again: ";
            std::cin >> id_term;
        }

        {
            std::lock_guard<std::mutex> lock(data.mutex);
            data.done_count = 0;
            data.terminate_flags[id_term] = true;
            std::cout << "Thread " << id_term << " completed.\n\n";
            data.cv_decision.notify_all();
        }

        if (threads[id_term].joinable()) {
            threads[id_term].join();
        }

        {
            std::lock_guard<std::mutex> lock(data.mutex);
            data.active_threads--;
        }
    }

    std::cout << "Resulting array: ";
    for (double val : data.arr) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";

    std::cout << "All threads completed. Program finished.\n";
    system("pause");
	return 0;
}
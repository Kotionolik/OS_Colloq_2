#include <gtest/gtest.h>
#include <thread>
#include <condition_variable>
#include "ArrayData.h"

class SingletonTest : public ::testing::Test {
protected:
    void TearDown() override {
        auto& data = SingletonArray::Instance();
        data.n = 0;
        data.k = 0;
        data.arr.clear();
        data.terminate_flags.clear();
        data.done_count = 0;
        data.active_threads = 0;
        data.start = false;
    }
};

TEST_F(SingletonTest, SingleInstance) {
    ArrayData& instance1 = SingletonArray::Instance();
    ArrayData& instance2 = SingletonArray::Instance();
    EXPECT_EQ(&instance1, &instance2) << "Singleton should return the same instance";
}

TEST_F(SingletonTest, ThreadSafeCreation) {
    constexpr int num_threads = 10;
    std::vector<ArrayData*> instances(num_threads);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i] {
            instances[i] = &SingletonArray::Instance();
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    ArrayData* first = &SingletonArray::Instance();
    for (int i = 0; i < num_threads; i++) {
        EXPECT_EQ(first, instances[i]) << "All threads should get the same instance";
    }
}

TEST_F(SingletonTest, ArrayDataInitialization) {
    ArrayData& data = SingletonArray::Instance();
    constexpr int n = 5;
    constexpr int k = 3;

    data.Initialize(n, k);

    EXPECT_EQ(data.n, n);
    EXPECT_EQ(data.k, k);
    EXPECT_EQ(data.arr.size(), n);
    EXPECT_EQ(data.terminate_flags.size(), k);
    EXPECT_FALSE(data.start);
    EXPECT_EQ(data.done_count, 0);
    EXPECT_EQ(data.active_threads, k);
}

TEST(AverageSetterTest, CalculationLogic) {
    ArrayData data;
    data.n = 5;
    data.arr = { 1, 2, 3, 4, 5 };

    int id = 2;
    int index = id % data.n;

    double sum = 0;
    for (double val : data.arr) {
        sum += val;
    }
    double avg = sum / data.arr.size();

    data.arr[index] = avg;

    EXPECT_DOUBLE_EQ(avg, 3.0);
    EXPECT_DOUBLE_EQ(data.arr[2], 3.0);

    EXPECT_DOUBLE_EQ(data.arr[0], 1.0);
    EXPECT_DOUBLE_EQ(data.arr[1], 2.0);
    EXPECT_DOUBLE_EQ(data.arr[3], 4.0);
    EXPECT_DOUBLE_EQ(data.arr[4], 5.0);
}

TEST_F(SingletonTest, ThreadSynchronization) {
    ArrayData& data = SingletonArray::Instance();
    data.Initialize(3, 2);

    std::thread t1(Average_setter, 0);
    std::thread t2(Average_setter, 1);

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.start = true;
        data.cv_start.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cv_all_done.wait_for(lock, std::chrono::seconds(1), [&] {
            return data.done_count == data.active_threads;
            });
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        EXPECT_EQ(data.done_count, 2);
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.terminate_flags[0] = true;
        data.terminate_flags[1] = true;
        data.cv_decision.notify_all();
    }

    t1.join();
    t2.join();
}

TEST_F(SingletonTest, ThreadTermination) {
    ArrayData& data = SingletonArray::Instance();
    data.Initialize(3, 3);

    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back(Average_setter, i);
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.start = true;
        data.cv_start.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cv_all_done.wait_for(lock, std::chrono::seconds(1), [&] {
            return data.done_count == data.active_threads;
            });
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.terminate_flags[1] = true;
        data.done_count = 0;
        data.cv_decision.notify_all();
    }

    if (threads[1].joinable()) {
        threads[1].join();
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.active_threads--;
        EXPECT_EQ(data.active_threads, 2);
        EXPECT_TRUE(data.terminate_flags[1]);
        EXPECT_FALSE(data.terminate_flags[0]);
        EXPECT_FALSE(data.terminate_flags[2]);
    }

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.terminate_flags[0] = true;
        data.terminate_flags[2] = true;
        data.cv_decision.notify_all();
    }

    threads[0].join();
    threads[2].join();
}

TEST_F(SingletonTest, MultipleIterations) {
    ArrayData& data = SingletonArray::Instance();
    data.Initialize(2, 1);
    data.arr = { 10, 20 };

    std::thread t(Average_setter, 0);

    {
        std::lock_guard<std::mutex> lock(data.mutex);
        data.start = true;
        data.cv_start.notify_all();
    }

    {
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cv_all_done.wait(lock, [&] { return data.done_count == 1; });

        EXPECT_DOUBLE_EQ(data.arr[0], 15.0);
        EXPECT_DOUBLE_EQ(data.arr[1], 20.0);

        data.done_count = 0;
        data.cv_decision.notify_one();
    }

    {
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cv_all_done.wait(lock, [&] { return data.done_count == 1; });

        EXPECT_DOUBLE_EQ(data.arr[0], 17.5);
        EXPECT_DOUBLE_EQ(data.arr[1], 20.0);

        data.terminate_flags[0] = true;
        data.cv_decision.notify_one();
    }

    t.join();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
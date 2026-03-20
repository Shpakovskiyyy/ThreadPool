#include <tp/ThreadPool.h>
#include <iostream>
#include <chrono>

int main()
{
    tp::ThreadPoolSpecs Specs(4, 1024);
    tp::ThreadPool Pool(Specs);

    Pool.TrySubmit([]()
    {
        std::cout << "Task A on thread: " << std::this_thread::get_id() << "\n";
    });

    auto Future = Pool.TrySubmit([](int A, int B)
    {
        return A + B;
    }, 21, 21);

    std::cout << "21 + 21 = " << Future.get() << "\n";

    auto ErrorFuture = Pool.TrySubmit([]()
    {
        throw std::runtime_error("Something went wrong");
        return 0;
    });

    try
    {
        ErrorFuture.get();
    }
    catch (const std::exception& E)
    {
        std::cout << "Caught exception: " << E.what() << "\n";
    }

    for (int i = 0; i < 16; i++)
    {
        Pool.TrySubmit([i]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::cout << "Task " << i << " on thread: " << std::this_thread::get_id() << "\n";
        });
    }

    Pool.Stop(tp::EThreadPoolShutdownPolicy::WaitAll);
    std::cout << "All tasks completed\n";

    return 0;
}
# tp::ThreadPool

A lightweight C++20 header-only thread pool with work stealing.

## Features

- Header-only — just include and use
- Work stealing for balanced load distribution
- `std::future` support for task results and exception handling
- Graceful and forced shutdown policies

## Requirements

- C++20
- CMake 3.20+

## Usage
```cpp
#include <tp/ThreadPool.h>

tp::ThreadPool Pool(tp::ThreadPoolSpecs(4, 1024));

// void task
Pool.TrySubmit([]() { std::cout << "Hello from thread pool\n"; });

// task with result
auto Future = Pool.TrySubmit([](int A, int B) { return A + B; }, 21, 21);
std::cout << Future.get() << "\n"; // 42

// shutdown
Pool.Stop(tp::EThreadPoolShutdownPolicy::WaitAll);
```

## Integration

### CMake
```cmake
add_subdirectory(ThreadPool)
target_link_libraries(YourTarget PRIVATE tp::ThreadPool)
```

## License

MIT

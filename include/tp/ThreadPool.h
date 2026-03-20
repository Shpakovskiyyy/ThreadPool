#pragma once

#include "TaskQueue.h"
#include "ThreadPoolSpecs.h"

#include <vector>
#include <algorithm>
#include <functional>
#include <future>
#include <random>


namespace tp
{
	inline constexpr auto KnuthConstant = 2654435761u;
	
	class ThreadPool
	{
		struct WorkerThread
		{
			public:
			WorkerThread()=default;
			~WorkerThread();
			
			public:
			WorkerThread(const WorkerThread&)				= delete;
			WorkerThread& operator=(const WorkerThread&)	= delete;
			
			public:
			WorkerThread(WorkerThread&&)					= delete;
			WorkerThread&& operator=(WorkerThread&&)		= delete;
			
			public:
			void Stop();
			
			public:
			std::thread Thread;
		};
		
		public:
		ThreadPool(const ThreadPoolSpecs& InThreadPoolSpecs);
		~ThreadPool();
	
		public:
		void Stop(EThreadPoolShutdownPolicy InShutdownPolicy);
		
		public:
		template <typename TaskType, typename ... TaskArgs>
		std::future<std::invoke_result_t<TaskType, TaskArgs...>> TrySubmit(TaskType&& InTask, TaskArgs&&... InArgs);

		private:
		[[nodiscard]] uint32_t GetNextWorkerID();
		[[nodiscard]] std::optional<std::function<void()>> TryGetTask(uint32_t WorkerIndex);
		
		private:
		void ThreadFunction(uint32_t WorkerIndex);
		
		private:
		void WaitAll();
		bool AllQueuesAreEmpty();
	
		private:
		std::vector<std::unique_ptr<WorkerThread>> Workers;
		std::vector<TaskQueue> Queues;
		
		private:
		std::mutex Mutex;
		std::condition_variable TaskCondition;
		std::condition_variable WaitCondition;
		
		private:
		uint32_t ThreadNum = 0;
		
		private:
		std::atomic_uint32_t NextWorker = 0;
		std::atomic_uint32_t NumActiveTasks = 0;
		
		private:
		std::atomic_bool bStopRequested = false;
	
	};
} // namespace tp


/* Begin ThreadPool::WorkerThread */
inline tp::ThreadPool::WorkerThread::~WorkerThread()
{
	Stop();
}

inline void tp::ThreadPool::WorkerThread::Stop()
{
	if (Thread.joinable()) Thread.join();
}
/* End ThreadPool::WorkerThread */


/* Begin ThreadPool */
inline tp::ThreadPool::ThreadPool(const ThreadPoolSpecs& InThreadPoolSpecs) 
: ThreadNum(InThreadPoolSpecs.ThreadNum)
{
	Workers.reserve(InThreadPoolSpecs.ThreadNum);
	Queues.reserve(InThreadPoolSpecs.ThreadNum);
	
	for (uint32_t index = 0; index < ThreadNum; index++)
	{
		Workers.emplace_back(std::make_unique<WorkerThread>());
		Queues.emplace_back(InThreadPoolSpecs.TaskQueueCapacity);
	}
	for (uint32_t index = 0; index < ThreadNum; index++)
	{
		Workers[index]->Thread = std::thread(&ThreadPool::ThreadFunction, this, index);
	}
}

inline tp::ThreadPool::~ThreadPool()
{
	Stop(EThreadPoolShutdownPolicy::Force);
}

inline void tp::ThreadPool::Stop(EThreadPoolShutdownPolicy InShutdownPolicy)
{
	if (InShutdownPolicy==EThreadPoolShutdownPolicy::WaitAll) WaitAll();
		
	bStopRequested.store(true, std::memory_order_release);
	TaskCondition.notify_all();
	for (uint32_t index = 0; index < ThreadNum; index++)
	{
		Workers[index]->Stop();
	}
}

template <typename TaskType, typename ... TaskArgs>
std::future<std::invoke_result_t<TaskType, TaskArgs...>> tp::ThreadPool::TrySubmit(TaskType&& InTask, TaskArgs&&... InArgs)
{
	using ReturnType = std::invoke_result_t<TaskType, TaskArgs...>;
	
	auto Promise = std::make_shared<std::promise<ReturnType>>();
	auto Future = Promise->get_future();
	
	if (bStopRequested.load(std::memory_order_acquire))
	{
		Promise->set_exception(std::make_exception_ptr(std::runtime_error("Cannot submit task: ThreadPool is stopped")));
		return Future;
	}
	
	NumActiveTasks.fetch_add(1, std::memory_order_relaxed);
	
	{
		std::unique_lock Lock(Mutex);
		std::function<void()> Task = [this,
			Promise,
			Func = std::forward<TaskType>(InTask),
			...CapturedArgs = std::forward<TaskArgs>(InArgs)]() mutable
		{
			try
			{
				if constexpr (std::is_void_v<ReturnType>)
				{
					std::invoke(Func, CapturedArgs...);
					Promise->set_value();
				}
				else
				{
					Promise->set_value(std::invoke(Func, CapturedArgs...));
				}
			}
			catch (...) { Promise->set_exception(std::current_exception()); }
			
			NumActiveTasks.fetch_sub(1, std::memory_order_relaxed);
			if (NumActiveTasks.load(std::memory_order_acquire) == 0) WaitCondition.notify_all();
		};
		
		Queues[GetNextWorkerID()].Enqueue(std::move(Task));
	}
	
	TaskCondition.notify_all();
	return Future;
}

inline uint32_t tp::ThreadPool::GetNextWorkerID()
{
	return NextWorker.fetch_add(1, std::memory_order_relaxed) % ThreadNum;
}

inline std::optional<std::function<void()>> tp::ThreadPool::TryGetTask(uint32_t WorkerIndex)
{
	if (auto Task = Queues[WorkerIndex].PopFront()) return Task;

	auto VictimIndex = (WorkerIndex * KnuthConstant + 1) % ThreadNum;
	if (VictimIndex == WorkerIndex) VictimIndex = (VictimIndex + 1) % ThreadNum;

	return Queues[VictimIndex].PopBack();
}

inline void tp::ThreadPool::ThreadFunction(uint32_t WorkerIndex)
{
	while (true)
	{
		std::optional<std::function<void()>> Task;
		{
			std::unique_lock Lock(Mutex);
			TaskCondition.wait(Lock, [this]
			{
				return !AllQueuesAreEmpty() || bStopRequested.load(std::memory_order_acquire);
			});

			if (bStopRequested.load(std::memory_order_acquire)) return;
			Task = TryGetTask(WorkerIndex);
		}

		if (Task.has_value() && Task.value()) Task.value()();
	}
}

inline void tp::ThreadPool::WaitAll()
{
	std::unique_lock Lock(Mutex);
	WaitCondition.wait(Lock, [this]
	{
		return NumActiveTasks.load(std::memory_order_acquire) == 0 || bStopRequested.load(std::memory_order_acquire);
	});
}

inline bool tp::ThreadPool::AllQueuesAreEmpty()
{
	return std::ranges::all_of(Queues, [](const TaskQueue& Queue) { return Queue.IsEmpty(); });
}
/* End ThreadPool */
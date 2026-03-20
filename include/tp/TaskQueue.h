#pragma once

#include <deque>
#include <mutex>
#include <functional>
#include <optional>
#include <stdexcept>


namespace tp
{
	class TaskQueue
	{
		public:
		explicit TaskQueue(uint32_t InTaskQueueCapacity);
		
		public:
		void Enqueue(std::function<void()>&& InTask);
	
		public:
		[[nodiscard]] std::optional<std::function<void()>> PopBack();
		[[nodiscard]] std::optional<std::function<void()>> PopFront();
		
		public:
		[[nodiscard]] bool IsEmpty() const;
	
		private:
		std::deque<std::function<void()>> Tasks;
		
		private:
		uint32_t TaskQueueCapacity;
	
	};
} // namespace tp


inline tp::TaskQueue::TaskQueue(uint32_t InTaskQueueCapacity) : TaskQueueCapacity(InTaskQueueCapacity) 
{}

inline void tp::TaskQueue::Enqueue(std::function<void()>&& InTask)
{
	if (Tasks.size() >= TaskQueueCapacity) throw std::length_error("TaskQueue is full");
	Tasks.push_back(std::move(InTask));
}

inline std::optional<std::function<void()>> tp::TaskQueue::PopBack()
{
	std::optional<std::function<void()>> Task{std::nullopt};
	if (!Tasks.empty())
	{
		Task = std::move(Tasks.back());
		Tasks.pop_back();
	}
	return Task;
}

inline std::optional<std::function<void()>> tp::TaskQueue::PopFront()
{
	if (Tasks.empty()) return std::nullopt;
	auto Task = std::move(Tasks.front());
	Tasks.pop_front();
	return Task;
}

inline bool tp::TaskQueue::IsEmpty() const
{
	return Tasks.empty();
}
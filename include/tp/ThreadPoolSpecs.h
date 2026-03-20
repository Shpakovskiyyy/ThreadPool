#pragma once

#include <algorithm>
#include <thread>
#include <cstdint>

namespace tp
{
	enum class EThreadPoolShutdownPolicy : uint8_t
	{
		Force,
		WaitAll,
		MAX
	};
	
	struct ThreadPoolSpecs
	{
		public:
		ThreadPoolSpecs(uint32_t InThreadNum, uint32_t InTaskQueueSize) 
		:	ThreadNum(std::clamp(InThreadNum, 1u, std::thread::hardware_concurrency())),
			TaskQueueCapacity(std::clamp(InTaskQueueSize, 1u, 1024u)) 
		{}
	
		public:
		uint32_t ThreadNum = 0;
		uint32_t TaskQueueCapacity = 0;
	};
}
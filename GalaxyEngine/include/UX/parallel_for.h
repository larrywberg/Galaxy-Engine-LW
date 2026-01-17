#pragma once

#include <algorithm>
#include <thread>
#include <vector>

inline int clamp_thread_count(size_t work_items, int requested_threads) {
	if (requested_threads <= 1 || work_items <= 1) {
		return 1;
	}
	return std::max(1, std::min<int>(requested_threads, static_cast<int>(work_items)));
}

template <typename Func>
inline void parallel_for(size_t begin, size_t end, int requested_threads, Func&& func) {
	const size_t count = (end > begin) ? (end - begin) : 0;
	const int thread_count = clamp_thread_count(count, requested_threads);
	if (thread_count <= 1) {
		for (size_t i = begin; i < end; ++i) {
			func(i, 0);
		}
		return;
	}

	const size_t chunk = (count + static_cast<size_t>(thread_count) - 1) / static_cast<size_t>(thread_count);
	std::vector<std::thread> workers;
	workers.reserve(static_cast<size_t>(thread_count));

	for (int t = 0; t < thread_count; ++t) {
		const size_t start = begin + static_cast<size_t>(t) * chunk;
		const size_t finish = std::min(end, start + chunk);
		if (start >= finish) {
			break;
		}
		workers.emplace_back([start, finish, t, &func]() {
			for (size_t i = start; i < finish; ++i) {
				func(i, t);
			}
		});
	}

	for (auto& worker : workers) {
		worker.join();
	}
}

#ifndef __CONCCURENCYQUEUE_HPP__
#define __CONCCURENCYQUEUE_HPP__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class ConcurrencyQueue {
 public:
  ConcurrencyQueue() = default;
  ~ConcurrencyQueue() = default;

  T pop() {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    auto item = _queue.front();
    _queue.pop();
    return item;
  }

  void pop(T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    item = _queue.front();
    _queue.pop();
  }

  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
    mlock.unlock();
    _cond.notify_one();
  }

  void push(T&& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(std::move(item));
    mlock.unlock();
    _cond.notify_one();
  }

 private:
  std::queue<T> _queue;
  std::mutex _mutex;
  std::condition_variable _cond;
};

#endif

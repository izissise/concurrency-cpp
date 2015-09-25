#ifndef __CONCCURENCYQUEUE_HPP__
#define __CONCCURENCYQUEUE_HPP__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

//! Implement a thread safe multiple consumer/producer queue (using mutexes)
//! @tparam T The underlying type the queue store;
//! @tparam CONTAINER The underlying container
template<typename T, class CONTAINER = std::queue<T>>
class ConcurrencyQueue {
 public:
  //! ConcurrencyQueue constructor
  ConcurrencyQueue() = default;

  //! ConcurrencyQueue destructor
  ~ConcurrencyQueue() = default;

  //! Wait until an element is in the queue
  //! @return The elements at the top of the queue
  T pop() {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    auto item = _queue.front();
    _queue.pop();
    return item;
  }

  //! Wait until an element is in the queue
  //! @param item A reference on the object that will stock the top element
  void pop(T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    item = _queue.front();
    _queue.pop();
  }

  //! Push an element in the queue
  //! @param item The element to push in the queue
  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
    mlock.unlock();
    _cond.notify_one();
  }

  //! Push an element in the queue but with move semantic
  //! @param item The element to push in the queue
  void push(T&& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(std::move(item));
    mlock.unlock();
    _cond.notify_one();
  }

 private:
  CONTAINER _queue;  //! The container
  std::mutex _mutex;  //! The mutex to share ressources
  std::condition_variable _cond;  //! The cond var to wait until somethin is on the queue
};

#endif

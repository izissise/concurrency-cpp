#ifndef __CONCURRENTHPP__
#define __CONCURRENTHPP__

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tupleApply.hpp"

#include "ConcurrencyQueue.hpp"

//! An helper function to set the value on a promise even if a function return void
template<typename RET, typename F, typename... Types>
inline void promise_set_value(std::promise<RET>& p, F& f, Types&&... ts) {
  p.set_value(f(std::forward<Types&&>(ts)...));
}

template<typename F, typename... Types>
inline void promise_set_value(std::promise<void>& p, F& f, Types&&... ts) {
  f(std::forward<Types&&>(ts)...);
  p.set_value();
}

template<typename RET, typename F, typename Tuple>
inline void promise_set_value(std::promise<RET>& p, F& f, Tuple&& t) {
  p.set_value(apply_tuple(f, t));
}

template<typename F, typename Tuple>
inline void promise_set_value(std::promise<void>& p, F& f, Tuple&& t) {
  apply_tuple(f, t);
  p.set_value();
}


//! Time helpers
using Clock = std::chrono::system_clock;
using Ms = std::milli;
using Sec = std::ratio<1>;

template<class Type, class Scale>
using Duration = std::chrono::duration<Type, Scale>;

template<class Type, class Scale>
using TimePoint = std::chrono::time_point<Clock, Duration<Type, Scale>>;


//! A class that implement a worker thread which allow to share ressources and defer
//! the work on that thread.
//! @tparam Types The ressources types;
template<class... Types>
class Concurrent {
 public:

  //! Concurrent constructor
  //! @param tasksSecond The maximum number of tasks that should be executed per seconds
  //! @param ts The shared ressources values
  explicit Concurrent(double tasksSecond, Types&&... ts)
  : _done(false),
  _t(std::forward_as_tuple(ts...)) {
    if (tasksSecond == 0.0) {
      _thread = std::thread(std::bind(&Concurrent::_executor, this));
    } else {
      _secondsPerTask = 1.0 / tasksSecond;
      _thread = std::thread(std::bind(&Concurrent::_executorChrono, this));
    }
  }

  //! Concurrent Destructor
  //! It assure that the worker thread is done before destroying the object
  ~Concurrent() {
    _queue.push([this](){ _done = true; });
    _thread.join();
  }

  //! Allow to push new function to execute concurrently on the shared ressources
  //! @param f The function that shall be called (it must take the shared ressource as parameter)
  //! @return An std::future of the result of f
  template<typename F,
  typename RET = typename std::result_of<F(Types...)>::type,
  int ...I>
  auto operator()(F f) -> std::future<RET> {
    auto promise = std::make_shared<std::promise<RET>>();
    auto future = promise->get_future();

    _queue.push([this, f, promise](){
      auto& p = *promise;
      try {
        promise_set_value(p, f, _t);
      } catch (...) {
        p.set_exception(std::current_exception());
      }
    });
    return future;
  }

  //! Set the new shared ressources
  //! @param ts The ressources
  //! @return The previous ressources as a tuple
  std::tuple<Types...> setSharedRessource(Types&&... ts) {
    auto nTuple = std::forward_as_tuple(ts...);
    std::swap(_t, nTuple);
    return nTuple;
  }

  //! Get the shared ressources
  //! @return The ressources as a tuple
  const std::tuple<Types...>& getSharedRessource() const {
    return _t;
  }

 protected:
  void _executor() {
    while (!_done) {
     _queue.pop()();
    }
  }

  void _executorChrono() {
    double taskTimeSec = 0.0;
    Duration<double, Ms> timeToSleep;
    TimePoint<double, Ms> start;
    TimePoint<double, Ms> end;
    start = std::chrono::system_clock::now();
    while (!_done) {
      _queue.pop()();
      end = std::chrono::system_clock::now();
      Duration<double, Ms> elapsed_milliseconds = start - end;
      taskTimeSec = elapsed_milliseconds.count() / 1000.0;
      timeToSleep = std::chrono::milliseconds(0);
      if (taskTimeSec < _secondsPerTask) {
        timeToSleep = std::chrono::milliseconds(static_cast<int>((_secondsPerTask - taskTimeSec) * 1000.0));
        std::this_thread::sleep_for(timeToSleep);
      }
      start = end + timeToSleep;
    }
  }
  
 private:
  double _secondsPerTask;
  bool _done;
  std::tuple<Types...> _t;
  ConcurrencyQueue<std::function<void()>> _queue;
  std::thread _thread;
};

#endif

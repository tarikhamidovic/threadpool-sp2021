#ifndef thread_pool_
#define thread_pool_ 
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <vector>
#include <atomic>
#include <type_traits>
#include <iostream>
#include <stdexcept>

using function_t = std::function<void()>;

class task_queue {
  public:
    bool blocking_pop(function_t& fun) {
      std::unique_lock<std::mutex> queue_lock{mtx_};
      while ( tasks_.empty() ) {
        if ( stopped_ ) return false;
        cv_.wait(queue_lock);
      }
      fun = std::move(tasks_.front());
      tasks_.pop();
      return true;
    }

    template<typename F>
      void push(F&& f) {
        {
          std::unique_lock<std::mutex> queue_lock{mtx_};
          tasks_.emplace(std::forward<F>(f));
        }
        cv_.notify_one();
      }

    void stop() {
      {
        std::unique_lock<std::mutex> queue_lock{mtx_};
        stopped_ = true;
      }
      cv_.notify_all();
    }

  private:
    std::queue<function_t> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stopped_ = false;
};

class thread_pool {
  public:
    thread_pool(size_t th_num = std::thread::hardware_concurrency()) : thread_number_{th_num}, counter{0} {
      // Inicijaliziramo vektor praznih task redova. Task queue i thread su asocirani istim indexom
      // u vektorima koji ih sadrze.
      tasks_ = std::vector<task_queue>(thread_number_);
      for(int i = 0; i < thread_number_; ++i)
        threads_.emplace_back( std::thread{ [this, i](){ run(i); }});
    }

    ~thread_pool() {
      for (int i = 0; i < threads_.size(); i++) {
        tasks_[i].stop(); // moramo stopirati task queue za svaki thread
        threads_[i].join();
      }
    }

    template<typename T>
      void async(T&& fun) {
        int next = round_robin_count();
        tasks_[next].push(std::forward<T>(fun));
      }

  private:
    /*
     * Dodajemo parametar th_index na osnovu kojeg se moze zakljuciti
     * koji thread da preuzme funkciju iz svog asociranog task queue-a
     * (thread index = task queue index)
     */
    void run(const int& th_index) {
      while(true) {
        function_t fun;
        if( !tasks_[th_index].blocking_pop(fun) )
          return;
        fun();
      }
    }

    /*
     * Funkcija koja racuna sljedeci index za thread
     * po Round Robin algoritmu. U slucaju da counter dobije
     * vrijednost broja thread-ova, postavlja se na nulu da ne bi 
     * vrijednost isla do beskonacnosti.
     */
    int round_robin_count() {
      int next = counter++ % thread_number_;
      if (counter >= thread_number_) counter = 0;
      return next;
    } 

    const size_t thread_number_;
    std::vector<std::thread> threads_;
    std::vector<task_queue> tasks_; // vektor task redova, svaki thread dobije svoj task queue
    size_t counter; // counter potreban za Round Robin algoritam
};

#endif /* ifndef thread_pool_ */


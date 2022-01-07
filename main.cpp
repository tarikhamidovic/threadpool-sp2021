#include "thread_pool_zadatak2.h"
#include <chrono>

std::vector<int> calculated_values;
std::mutex mtx;

void add_calculated_value(int v) {
  std::unique_lock<std::mutex> result_lock(mtx);
  calculated_values.push_back(v);
}

void print_calculated_values() {
  std::unique_lock<std::mutex> result_lock(mtx);
  for(const auto &el : calculated_values)
    std::cout<<el<<std::endl;
}

int fib(size_t n) {
  if (n == 0) 
    return 0;
  if ( n == 1 )
    return 1;
  return fib(n-1)+fib(n-2);
}

int call_fib(size_t n) {
  int result = fib(n);
  add_calculated_value(result);
  return result;
}


int main(int argc, char *argv[])
{
  thread_pool tp;

  for(int i = 0; i < 40; ++i) {
    tp.async(std::bind(call_fib, i));
  }

  // for(int i = 0; i < 40; ++i) {
  //   tp.async( [i](){ call_fib(i);} );
  // }

  std::this_thread::sleep_for(std::chrono::seconds{5});
  print_calculated_values();

  return 0;
}

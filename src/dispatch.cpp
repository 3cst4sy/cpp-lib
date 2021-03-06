//
// Copyright 2015 KISS Technologies GmbH, Switzerland
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "cpp-lib/dispatch.h"


// Make sure that tasks is initialized before the thread is
// started!
// Initialize thread pool
cpl::dispatch::thread_pool::thread_pool(int const n_threads)
  : tasks{} {
  cpl::util::verify(n_threads >= 1, 
      "thread pool: must have at least one worker thread");
  workers.reserve(n_threads);
  for (int i = 0; i < n_threads; ++i) {
    workers.push_back(std::thread(&thread_pool::thread_function, this));
  }
}

// TODO: allow detaching?
cpl::dispatch::thread_pool::~thread_pool() {
  // Signal 'EOF' to all workers---one thread will pop
  // only one task wiht continue set to false
  for (int i = 0; i < num_workers(); ++i) {
    tasks.push(task_and_continue{[]{}, false});
  }
  // Join all workers
  for (int i = 0; i < num_workers(); ++i) {
    workers[i].join();
  }
}

void cpl::dispatch::thread_pool::dispatch(cpl::dispatch::task&& t) {
  tasks.push(task_and_continue{std::move(t), true});
}

void cpl::dispatch::thread_pool::thread_function() {
  do {
    auto const tac = tasks.pop_front();
    if (!tac.second) {
      return;
    } else {
      tac.first();
    }
  } while (true);
}

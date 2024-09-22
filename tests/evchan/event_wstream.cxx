#include <array>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <thread>
#include <map>
#include <utility>

#include <tarp/evchan.hxx>

#include "event_stream_test.hxx"

using namespace std;
using namespace std::chrono_literals;
namespace E = tarp::evchan::ts;

// helper for consumer/producer threads to get notifications.
struct wait_struct : public E::interfaces::notifier{
    tarp::binary_semaphore sem;
    bool notify(uint32_t, uint32_t) override{
        sem.release();
        return true;
    }
};

// Stream num_msgs per producer as fast as possible. One single consumer
// will increment a counter for each message received. Use a buffer of the
// specified size for the stream channel.
//
// Note in this test each producer corresponds to a separate thread; therefore
// since there is a maximum system limit to the number of threads, there is a
// limit to the number of producers.
//
// Each producer will produce a new element at producer_period intervals.
bool test_event_wstream(
        uint32_t num_msgs_per_producer,
        uint32_t num_producers,
        unsigned buffsz,
        std::chrono::seconds max_wait,
        std::chrono::microseconds producer_period
        )
{
    E::event_wstream<std::unique_ptr<unsigned>> s(buffsz);
    std::size_t counter{0};
    std::map<unsigned, thread> producers;

  bool test_passed{true};

  auto start_producers = [&](){
    for (uint32_t i = 0; i < num_producers; ++i) {
        auto chan = s.interface().channel();

      thread t([chan, &num_msgs_per_producer, &producer_period]() {
        //std::cerr << "Consumer " << i << " initialized with counter address " << std::addressof(counter) << std::endl;
        for (unsigned j =0; j < num_msgs_per_producer; ++j){
        //cerr << "chan get waiting\n";
            
            std::this_thread::sleep_for(producer_period);
            auto msg = std::make_unique<unsigned>(j);
            chan->try_push(std::move(msg));
        //cerr << "after chan get waiting\n";
        }
      });

      producers.insert(std::make_pair(i, std::move(t)));
    }
  };

  auto join_threads = [](auto &ls) {
    for (auto &[k, t] : ls) {
        //cerr << "joining thread " << k <<endl;
        t.join();
    }
  };

  cerr << "starting producers\n";
  start_producers();

  std::cerr << "RUNNING" << std::endl;

    E::monitor mon;
    mon.watch(s, E::chanState::READABLE, 0);
    
    auto deadline = std::chrono::steady_clock::now() + max_wait;
    while(chrono::steady_clock::now() < deadline){
        // done
        if (counter >= num_producers * num_msgs_per_producer){
            break;
        }
        auto evs = mon.wait_until(deadline);
        for (auto &_: evs){
            (void)_; // unused.
            auto elems = s.get_all();
            counter += elems.size();
        }
    }

    //cerr << "closing channels\n";
    s.close();

    cerr << "joining threads\n";
    join_threads(producers);

    //
    // PRINTS
    //
    std::cerr << std::dec << std::endl;
    std::cerr << "Asked to stream: " << num_msgs_per_producer << " msgs per producer" << std::endl;
    std::cerr << "Producers: " << num_producers << endl;

    test_passed = counter == (num_msgs_per_producer * num_producers);
    std::cerr << "Received: " << counter << endl;

  return test_passed;
}


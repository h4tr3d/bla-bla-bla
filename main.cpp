#include <deque>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <numeric>

#include <iostream>

namespace hotel_processing {

// fwd
namespace priv {
struct hotel;
} // ::priv

using time_t       = int64_t;
using client_id_t  = uint32_t;
//using room_t       = uint16_t;
using room_t       = size_t;
using hotels_map_t = std::unordered_map<std::string, priv::hotel>;

//constexpr time_t start_time;
constexpr time_t TIME_WINDOW = 24*60*60;

namespace priv {
struct booking
{
    time_t      time;
    client_id_t client;
    room_t      rooms;
};

#define CACHED
#ifdef CACHED
struct hotel
{
    void book(booking&& info)
    {
        setup(info.rooms, info.client);
        m_bookings.push_back(std::move(info));
    }

    size_t clients(time_t current_time)
    {
        remove_old(current_time);
        return m_client_bookings.size();
    }

    size_t rooms(time_t current_time)
    {
        remove_old(current_time);
        return m_rooms;
    }

private:
    void setup(room_t rooms, client_id_t client)
    {
        m_rooms += rooms;
        ++m_client_bookings[client];
    }

    void cleanup(room_t rooms, client_id_t client)
    {
        m_rooms -= rooms;
        auto client_it = m_client_bookings.find(client);
        if (--client_it->second == 0) {
            m_client_bookings.erase(client_it);
        }
    }

    // Remove old entries
    void remove_old(time_t current_time)
    {
        if (m_bookings.empty()) {
            return;
        }

        auto it = m_bookings.begin();
        for (; it != m_bookings.end(); ++it) {
            if (it->time > (current_time - TIME_WINDOW)) {
                break;
            }
            cleanup(it->rooms, it->client);
        }

        if (it != m_bookings.begin())
            m_bookings.erase(m_bookings.begin(), it);
    }

private:
    //
    std::deque<booking> m_bookings;
    // Cache
    std::unordered_map<client_id_t, size_t> m_client_bookings;
    size_t m_rooms{};
};
#else
struct hotel
{
    void book(booking&& info)
    {
        m_bookings.push_back(std::move(info));
    }

    size_t clients(time_t current_time)
    {
        remove_old(current_time);

        auto tmp = m_bookings;
        std::sort(tmp.begin(), tmp.end(), [](auto const& a, auto const& b) {
            return a.client < b.client;
        });
        auto last = std::unique(tmp.begin(), tmp.end(), [](auto const& a, auto const& b) {
            return a.client == b.client;
        });
        return std::distance(tmp.begin(), last);
    }

    size_t rooms(time_t current_time)
    {
        remove_old(current_time);
        return std::accumulate(m_bookings.begin(), m_bookings.end(), size_t(0), [](auto prev, auto const& info) {
            return prev + info.rooms;
        });
    }

private:
    // Remove old entries
    void remove_old(time_t current_time)
    {
        if (m_bookings.empty()) {
            return;
        }

        auto it = std::upper_bound(m_bookings.begin(), m_bookings.end(), current_time - TIME_WINDOW,
                                   [](auto tm, auto const& info) {
                                       return tm < info.time;
                                   });
        m_bookings.erase(m_bookings.begin(), it);
    }

private:
    std::deque<booking> m_bookings;
};
#endif

} // ::priv

class context
{
public:

    void book(time_t time, const std::string& hotel_name, client_id_t client_id, room_t room_count)
    {
        m_current_time = time;
        m_hotels[hotel_name].book({time, client_id, room_count});
    }

    size_t clients(const std::string& hotel_name)
    {
        return m_hotels[hotel_name].clients(m_current_time);
    }

    size_t rooms(const std::string& hotel_name)
    {
        return m_hotels[hotel_name].rooms(m_current_time);
    }

private:
    time_t       m_current_time{};
    hotels_map_t m_hotels;
};


} // ::hotel_processing

//#define TESTS
#ifdef TESTS

#include "tests.inc"

int main()
{
    test_runner tr;
    RUN_TEST(tr, Test1);
    RUN_TEST(tr, Test2);
    RUN_TEST(tr, Test3);
    RUN_TEST(tr, Test4);
    RUN_TEST(tr, Test5);
    RUN_TEST(tr, Test6);
    RUN_TEST(tr, TimeTest);

    return 0;
}
#else
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    hotel_processing::context ctx;

    size_t requests_count;
    std::cin >> requests_count;

    for (size_t i = 0; i < requests_count; ++i) {
        std::string request_kind;
        std::cin >> request_kind;

        if (request_kind == "BOOK") {
            hotel_processing::time_t time;
            hotel_processing::client_id_t client;
            hotel_processing::room_t rooms;
            std::string hotel_name;

            std::cin >> time >> hotel_name >> client >> rooms;

            ctx.book(time, hotel_name, client, rooms);

        } else {
            std::string hotel_name;
            std::cin >> hotel_name;

            if (request_kind == "CLIENTS") {
                std::cout << ctx.clients(hotel_name) << '\n';
            } else if (request_kind == "ROOMS") {
                std::cout << ctx.rooms(hotel_name) << '\n';
            }
        }

    }

    return 0;
}
#endif

#pragma once

#include <deque>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <numeric>

namespace hotel_processing {

using time_t       = int64_t;
using client_id_t  = uint32_t;
using room_t       = size_t;

// fwd
namespace priv {
struct hotel;
} // ::priv

using hotels_map_t = std::unordered_map<std::string, priv::hotel>;

static inline constexpr time_t TIME_WINDOW = 24*60*60;

namespace priv {
struct booking
{
    time_t      time;
    client_id_t client;
    room_t      rooms;
};


#ifdef CACHED
struct hotel
{
    void book(booking&& info);

    size_t clients(time_t current_time);

    size_t rooms(time_t current_time);

private:
    void setup(room_t rooms, client_id_t client);

    void cleanup(room_t rooms, client_id_t client);

    // Remove old entries
    void remove_old(time_t current_time);

private:
    std::deque<booking> m_bookings;
    // Cache
    std::unordered_map<client_id_t, size_t> m_client_bookings;
    size_t m_rooms{};
};
#else
struct hotel
{
    void book(booking&& info);

    size_t clients(time_t current_time);

    size_t rooms(time_t current_time);

private:
    // Remove old entries
    void remove_old(time_t current_time);

private:
    std::deque<booking> m_bookings;
};
#endif

} // ::priv

class context
{
public:

    void book(time_t time, const std::string& hotel_name, client_id_t client_id, room_t room_count);

    size_t clients(const std::string& hotel_name);

    size_t rooms(const std::string& hotel_name);

private:
    time_t       m_current_time{};
    hotels_map_t m_hotels;
};


} // ::hotel_processing

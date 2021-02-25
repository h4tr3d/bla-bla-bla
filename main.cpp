#include <deque>
#include <unordered_map>
#include <map>
#include <algorithm>

#include <iostream>

namespace hotel_processing {

// fwd
namespace priv {
struct hotel;
} // ::priv

using time_t       = int64_t;
using client_id_t  = uint32_t;
using room_t       = uint16_t;
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

struct hotel
{
    void book(booking&& info)
    {
        m_bookings.emplace_back(std::move(info), this);
    }

    size_t clients(time_t current_time)
    {
        remove_old(current_time);
        return m_client_bookings.size();
    }

    room_t rooms(time_t current_time)
    {
        remove_old(current_time);
        return m_rooms;
    }

private:

    // Helper to setup and cleanup cache etries
    struct booking_wrap : booking
    {
        booking_wrap(const booking_wrap&) = delete;
        void operator=(const booking_wrap&) = delete;

        booking_wrap() = default;

        booking_wrap(booking_wrap&& other):
            booking_wrap()
        {
            swap(other);
        }

        booking_wrap& operator=(booking_wrap&& other)
        {
            booking_wrap(std::move(other)).swap(*this);
            return *this;
        }

        booking_wrap(booking&& other, hotel* parent) :
            booking(std::move(other)),
            parent(parent)
        {
            if (parent)
                parent->setup(rooms, client);
        }

        ~booking_wrap()
        {
            if (parent)
                parent->cleanup(rooms, client);
        }

        void swap(booking_wrap& other)
        {
            std::swap(parent, other.parent);
            std::swap(time, other.time);
            std::swap(client, other.client);
            std::swap(rooms, other.rooms);
        }

        hotel* parent = nullptr;
    };

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

#if 1
        auto it = std::upper_bound(m_bookings.begin(), m_bookings.end(), current_time - TIME_WINDOW,
                                   [](auto tm, auto const& booking) {
                                       return tm < booking.time;
                                   });
        m_bookings.erase(m_bookings.begin(), it);
#else
        auto it = m_bookings.begin();
        if (it->time > current_time - TIME_WINDOW)
            return;

        for (; it < m_bookings.end(); ++it) {
            if (it->time > current_time - TIME_WINDOW)
                break;

            cleanup(it->rooms, it->client);
        }

        m_bookings.erase(m_bookings.begin(), it);
#endif
    }

private:
    // Cache
    std::unordered_map<client_id_t, size_t> m_client_bookings;
    room_t m_rooms{};
    //
    std::deque<booking_wrap> m_bookings;
};
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

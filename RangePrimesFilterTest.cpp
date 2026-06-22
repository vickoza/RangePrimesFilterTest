#include <chrono>
#include <iostream>
#include <numeric>
#include <ranges>
#include <execution>
#include <vector>
#include <new>
#include <atomic>

constexpr auto N = 10'000'000;
std::atomic<long long> memUsing{0};
std::atomic<long long> totalMemUsed{ 0 };

void* operator new(std::size_t n) noexcept(false)
{
    void* p = std::malloc(n + sizeof(std::max_align_t));
    if (p == nullptr)
        throw std::bad_alloc{};
    std::max_align_t* q = new (p) std::max_align_t(n);
	memUsing +=  n + sizeof(std::max_align_t);
	totalMemUsed += sizeof(std::max_align_t) + sizeof(std::size_t);
    return q + 1;
}

void* operator new[](std::size_t n) noexcept(false)
{
    void* p = std::malloc(n + sizeof(std::max_align_t));
    if (p == nullptr)
        throw std::bad_alloc{};
    std::max_align_t* q = new (p) std::max_align_t(n);
	memUsing += n + sizeof(std::max_align_t);
	totalMemUsed += n + sizeof(std::max_align_t);;
    return q + 1;
}

void operator delete(void* ptr, std::size_t n) noexcept
{
    if (ptr == nullptr)
        return;
    auto q{ static_cast<std::max_align_t*>(ptr) - 1 };
    memUsing -= n + sizeof(std::max_align_t);;
    std::free(q);
}

void operator delete[](void* ptr, std::size_t n) noexcept
{
    if (ptr == nullptr)
        return;
    auto q{ static_cast<std::max_align_t*>(ptr) - 1 };
    memUsing -= n + sizeof(std::max_align_t);;
    std::free(q);
}

int main() {
    {
        auto t1 = std::chrono::steady_clock::now();
        //std::vector<int> primes1{};

        auto isprime_old = [](int n)
            {
                if (n < 2) return false;
                for (int i = 2; i * i <= n; ++i)
                {
                    if (n % i == 0) return false;
                }
                return true;
            };

        long long psum = 0;
        for (int i = 1; i < N; ++i) {
            if (isprime_old(i)) psum += i;
        }
        std::cout << psum << '\n';

        /*for (int i = 1; i < N; ++i)
        {
            if (isprime_old(i))
                primes1.emplace_back(i);
        }
        std::cout << std::reduce(std::execution::par, primes1.begin(), primes1.end(), 0ll) << '\n';*/

        auto t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt1 = t2 - t1;
        std::cout << dt1.count() << " seconds" << '\n';

        auto isprime_new = [](int n) {
            using namespace std::views;
            return n > 1 and (
                iota(2) |
                take_while([n](int k) { return k * k <= n; }) |
                filter([n](int k) { return n % k == 0; })
                ).empty();
            };

        auto primes = std::views::iota(1, N) | std::views::filter(isprime_new);
        std::cout << std::reduce(std::execution::par, primes.begin(), primes.end(), 0ll) << '\n';

        auto t3 = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt2 = t3 - t2;
        std::cout << dt2.count() << " seconds" << '\n';
    }
    std::cout << "Memory leaked: " << memUsing.load() << " bytes\n";
    std::cout << "Total memory used: " << totalMemUsed.load() << " bytes\n";

    return 0;
}
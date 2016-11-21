#if 0
#include <range/v3/view/iota.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/action/insert.hpp>
#include <vector>
#include <iostream>

using namespace ranges;

auto make_view()
{
    int sum = 0;
    return view::ints |
           view::take_while([sum](int val) mutable {
               sum += val;
               return sum <= 10;
           });
}

int main()
{
    CONCEPT_ASSERT(ranges::RandomAccessRange<decltype(make_view())>());

    std::cout << make_view() << "\n"; // prints [1,1,1,1,1]

    std::vector<int> copy_vec;
    copy(make_view(), ranges::back_inserter(copy_vec));
    copy(copy_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1 1

    std::cout << "\n";

    std::vector<int> push_back_vec;
    push_back(push_back_vec, make_view());
    copy(push_back_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";

    std::vector<int> insert_vec;
    insert(insert_vec, insert_vec.begin(), make_view());
    copy(insert_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";
}
#elif 0
#include <range/v3/view/repeat.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/action/insert.hpp>
#include <vector>
#include <iostream>

using namespace ranges;

auto make_view()
{
    int sum = 0;
    return view::repeat(1) |
           view::take_while([sum](int val) mutable {
               sum += val;
               return sum <= 5;
           });
}

int main()
{
    CONCEPT_ASSERT(ranges::RandomAccessRange<decltype(make_view())>());

    std::cout << make_view() << "\n"; // prints [1,1,1,1,1]

    std::vector<int> copy_vec;
    copy(make_view(), ranges::back_inserter(copy_vec));
    copy(copy_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1 1

    std::cout << "\n";

    std::vector<int> push_back_vec;
    push_back(push_back_vec, make_view());
    copy(push_back_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";

    std::vector<int> insert_vec;
    insert(insert_vec, insert_vec.begin(), make_view());
    copy(insert_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";
}
#else
#include <range/v3/view/generate.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/action/insert.hpp>
#include <vector>
#include <iostream>

using namespace ranges;

auto make_view()
{
    int sum = 0;
    return view::generate([]{ return 1; }) | view::take_while([sum](int val) mutable {
        sum += val;
        return sum <= 5;
    });
}

int main()
{
    std::vector<int> copy_vec;
    ranges::copy(make_view(), ranges::back_inserter(copy_vec));
    ranges::copy(copy_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1 1

    std::cout << "\n";

    std::vector<int> push_back_vec;
    ranges::push_back(push_back_vec, make_view());
    ranges::copy(push_back_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";

    std::vector<int> insert_vec;
    ranges::insert(insert_vec, insert_vec.begin(), make_view());
    ranges::copy(insert_vec, ostream_iterator<>(std::cout, " ")); // prints 1 1 1 1

    std::cout << "\n";
}
#endif

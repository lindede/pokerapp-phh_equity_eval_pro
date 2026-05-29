#include "combinations.h"
#include <gtest/gtest.h>

using namespace pokerstove;

TEST(Combinations, holdem)
{
    // if we run though all the 2 card combos we get 52c2 hands
    int count = 0;
    combinations cards(52, 2);
    do {
        count += 1;
    } while (cards.next());

    EXPECT_EQ(count, 1326);
}

TEST(Combinations, choose_values)
{
    EXPECT_EQ(choose(52, 2), 1326u);
    EXPECT_EQ(choose(4, 8), 0u);
    EXPECT_EQ(choose(10, 0), 1u);
    EXPECT_EQ(choose(10, 10), 1u);
}

TEST(Combinations, time_choose)
{
    for (int i = 0; i < 6000; i++) {
        for (int k = 0; k < 10; k++) {
            for (int n = k; n < 10; n++) {
                (void)choose(n, k);
            }
        }
    }
}

#include <iostream>
#include <map>
#include <pokerstove/peval/Card.h>
#include <pokerstove/peval/CardSet.h>
#include <pokerstove/util/argparse.hpp>
#include <pokerstove/util/combinations.h>
#include <pokerstove/util/format.hpp>
#include <set>
#include <string>

using namespace std;
using namespace pokerstove;

#if 0
std::set<CardSet>
expandRankSet(size_t numCards)
{
    combinations cards(52,numCards);
    do
    {
        CardSet hand;
        for (size_t i=0; i<num_cards; i++)
        {
            hand.insert (Card(cards[i]));
        }
        collection.insert(hand.canonize());
        rankHands[hand.rankstr()] = hand.rankColex();
    }
    while (cards.next());
}
#endif

int main(int argc, char** argv)
{
    try
    {
        ArgParser parser;
        parser.set_description(
            "pscolex, a utility which prints all combinations\n"
            "of poker hands, using canonical suits, or only ranks");
        parser.add_option("help", '?', false, {}, true);
        parser.add_option("num-cards", 'n', true, "2");
        parser.add_option("ranks", '\0', false, {}, true);

        if (!parser.parse(argc, argv) || parser.flag("help") || argc == 1)
        {
            parser.print_help(cout);
            return 1;
        }

        size_t num_cards = static_cast<size_t>(stoul(parser.get("num-cards")));

        set<CardSet> canonicalHands;
        map<string, size_t> rankHands;
        combinations cards(52, num_cards);
        do
        {
            CardSet hand;
            for (size_t i = 0; i < num_cards; i++)
                hand.insert(Card(cards[i]));
            canonicalHands.insert(hand.canonize());
            rankHands[hand.rankstr()] = hand.rankColex();
        } while (cards.next());

        if (parser.flag("ranks"))
        {
            for (auto it = rankHands.begin(); it != rankHands.end(); it++)
                cout << util::format("{}: {}\n", it->first, it->second);
        }
        else
        {
            for (auto it = canonicalHands.begin(); it != canonicalHands.end(); it++)
                cout << util::format("{}: {}\n", it->str(), it->colex());
        }
    }
    catch (std::exception& e)
    {
        cerr << "-- caught exception--\n" << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        cerr << "Exception of unknown type!\n";
        return 1;
    }
    return 0;
}

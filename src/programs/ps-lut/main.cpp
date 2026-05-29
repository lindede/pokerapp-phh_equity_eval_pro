#include <iostream>
#include <pokerstove/peval/Card.h>
#include <pokerstove/peval/CardSet.h>
#include <pokerstove/peval/CardSetGenerators.h>
#include <pokerstove/peval/PokerHandEvaluator.h>
#include <pokerstove/util/argparse.hpp>
#include <pokerstove/util/combinations.h>
#include <pokerstove/util/format.hpp>
#include <set>
#include <string>

using namespace std;
using namespace pokerstove;

int main(int argc, char** argv)
{
    try
    {
        ArgParser parser;
        parser.set_description("create a lookup table for poker hands");
        parser.add_option("help", '?', false, {}, true);
        parser.add_option("pocket-count", 'p', true, "2");
        parser.add_option("board-count", 'b', true, "3");
        parser.add_option("game", 'g', true, "O");
        parser.add_option("ranks", '\0', false, {}, true);

        if (!parser.parse(argc, argv) || parser.flag("help") || argc == 1)
        {
            parser.print_help(cout);
            return 1;
        }

        size_t pocketCount = static_cast<size_t>(stoul(parser.get("pocket-count")));
        size_t boardCount = static_cast<size_t>(stoul(parser.get("board-count")));
        string game = parser.get("game");

        bool ranks = parser.flag("ranks");
        Card::Grouping grouping = Card::SUIT_CANONICAL;
        if (ranks)
            grouping = Card::RANK;
        set<CardSet> pockets = createCardSet(pocketCount, grouping);
        set<CardSet> boards = createCardSet(boardCount, grouping);

        auto evaluator = PokerHandEvaluator::alloc(game);
        for (auto pit = pockets.begin(); pit != pockets.end(); pit++)
            for (auto bit = boards.begin(); bit != boards.end(); bit++)
            {
                if (ranks)
                {
                    PokerEvaluation eval = evaluator->evaluateRanks(*pit, *bit);
                    cout << util::format("{}, {}: [{:4d},{:4d}] -> {} [{:9d}]\n",
                                        pit->str(),
                                        bit->str(),
                                        pit->rankColex(),
                                        bit->rankColex(),
                                        eval.str(),
                                        eval.code());
                }
                else
                {
                    bit->canonize(*pit);
                    if (pit->intersects(*bit))
                        continue;
                    PokerHandEvaluation eval = evaluator->evaluate(*pit, *bit);
                    cout << util::format("{}, {}: [{:4d},{:4d}] -> {} [{:9d}]\n",
                                        pit->str(),
                                        bit->str(),
                                        pit->colex(),
                                        bit->colex(),
                                        eval.str(),
                                        eval.high().code());
                }
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

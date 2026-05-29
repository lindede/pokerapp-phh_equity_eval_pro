#include <iostream>
#include <pokerstove/peval/Card.h>
#include <pokerstove/penum/ShowdownEnumerator.h>
#include <pokerstove/util/argparse.hpp>
#include <pokerstove/util/format.hpp>
#include <pokerstove/util/timing.hpp>
#include <vector>

using namespace pokerstove;
using namespace std;

namespace
{

void print_usage_hint(ostream& out)
{
    out << "See USAGEA.md for supported hand syntax and limitations.\n";
}

bool parse_hand_distribution(CardDistribution& dist, const string& hand, size_t player_index)
{
    if (dist.parse(hand))
        return true;

    cerr << "error: invalid hand for player " << (player_index + 1) << ": \"" << hand << "\"\n";
    cerr << "       ps-eval only accepts explicit cards (e.g. AcAs), weighted combos, or \".\" for random.\n";
    cerr << "       Range notation such as 22+, AKo, QQ+ is not supported.\n";
    print_usage_hint(cerr);
    return false;
}

bool parse_board(CardSet& board, const string& board_str)
{
    board.clear();
    if (board_str.empty())
        return true;

    if (board_str.size() % 2 != 0)
    {
        cerr << "error: invalid board: \"" << board_str << "\"\n";
        cerr << "       Board length must be an even number of characters (rank+suit pairs).\n";
        print_usage_hint(cerr);
        return false;
    }

    for (size_t i = 0; i < board_str.size(); i += 2)
    {
        Card c;
        const string card_str = board_str.substr(i, 2);
        if (!c.fromString(card_str))
        {
            cerr << "error: invalid board card \"" << card_str << "\" in \"" << board_str << "\"\n";
            print_usage_hint(cerr);
            return false;
        }
        if (board.contains(c))
        {
            cerr << "error: duplicate board card \"" << card_str << "\"\n";
            return false;
        }
        board.insert(c);
    }
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    ArgParser parser;
    parser.set_description("ps-eval, a poker hand evaluator");
    parser.add_option("help", '?', false, {}, true);
    parser.add_option("game", 'g', true, "h");
    parser.add_option("board", 'b', true);
    parser.add_option("hand", 'h', true);
    parser.add_option("quiet", 'q', false, {}, true);
    parser.add_positional("hand");

    if (!parser.parse(argc, argv) || parser.flag("help") || argc == 1)
    {
        parser.print_help(cout);
        print_usage_hint(cout);
        return 1;
    }

    string game = parser.get("game");
    string board_str = parser.has("board") ? parser.get("board") : "";
    vector<string> hands = parser.positional();
    if (hands.empty() && parser.has("hand"))
        hands.push_back(parser.get("hand"));

    if (hands.empty())
    {
        cerr << "error: at least one hand is required.\n";
        print_usage_hint(cerr);
        return 1;
    }

    bool quiet = parser.flag("quiet");

    std::shared_ptr<PokerHandEvaluator> evaluator = PokerHandEvaluator::alloc(game);
    if (!evaluator)
    {
        cerr << "error: unknown game \"" << game << "\".\n";
        print_usage_hint(cerr);
        return 1;
    }

    CardSet board;
    if (!parse_board(board, board_str))
        return 1;

    vector<CardDistribution> handDists;
    handDists.reserve(hands.size());
    for (size_t i = 0; i < hands.size(); ++i)
    {
        handDists.emplace_back();
        if (!parse_hand_distribution(handDists.back(), hands[i], i))
            return 1;
    }

    if (handDists.size() == 1)
    {
        handDists.emplace_back();
        handDists.back().fill(evaluator->handSize());
    }

    ShowdownEnumerator showdown;
    const auto t0 = SteadyClock::now();
    vector<EquityResult> results =
        showdown.calculateEquity(handDists, board, evaluator);
    const double elapsed = elapsed_seconds(t0);

    double total = 0.0;
    for (const EquityResult& result : results)
        total += result.winShares + result.tieShares;

    if (!quiet)
    {
        for (size_t i = 0; i < results.size(); ++i)
        {
            double equity = (results[i].winShares + results[i].tieShares) / total;
            string handDesc =
                (i < hands.size()) ? "The hand " + hands[i] : "A random hand";
            cout << handDesc << " has " << equity * 100. << " % equity ("
                 << results[i].str() << ")" << endl;
        }
    }

    cerr << util::format("elapsed: {:.3f} s\n", elapsed);
}

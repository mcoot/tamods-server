#include <boost/test/unit_test.hpp>

#include "../MatchSummary.h"

BOOST_AUTO_TEST_CASE(MyTest)
{
    char buffer[1024];
    MatchSummary::OverallMatchStats overallStats;
    BOOST_CHECK_EQUAL(overallStats.size(), 7 * (2 + 4) + 1 * (2 + 8) + 2 * (2 + 2));
    overallStats.toBytes(buffer);
}

/* Copyright 2009 JetBrains
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * $Revision$
*/

#include <sstream>

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/results_collector.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include "teamcity_messages.h"

using namespace boost::unit_test;
using namespace std;

namespace JetBrains {

// Custom formatter for TeamCity messages (updated for Boost 1.87)
class TeamcityBoostLogFormatter: public boost::unit_test::unit_test_log_formatter {
    TeamcityMessages messages;
    std::string currentDetails;

public:
    TeamcityBoostLogFormatter();

    void log_start(std::ostream&, boost::unit_test::counter_t test_cases_amount) override;
    void log_finish(std::ostream&) override;
    void log_build_info(std::ostream&, bool = true) override;

    void test_unit_start(std::ostream&, boost::unit_test::test_unit const& tu) override;
    void test_unit_finish(std::ostream&,
        boost::unit_test::test_unit const& tu,
        unsigned long elapsed) override;
    void test_unit_skipped(std::ostream&, boost::unit_test::test_unit const& tu) override;

    void log_exception_start(std::ostream&,
        boost::unit_test::log_checkpoint_data const&,
        boost::execution_exception const& ex) override;
    void log_exception_finish(std::ostream&) override;

    void log_entry_start(std::ostream&,
        boost::unit_test::log_entry_data const&,
        log_entry_types let) override;
    void log_entry_value(std::ostream&, boost::unit_test::const_string value) override;
    void log_entry_finish(std::ostream&) override;

    void entry_context_start(std::ostream&, boost::unit_test::log_level) override;
    void log_entry_context(std::ostream&, boost::unit_test::log_level, boost::unit_test::const_string) override;
    void entry_context_finish(std::ostream&, boost::unit_test::log_level) override;
};

// Fake fixture to register formatter
struct TeamcityFormatterRegistrar {
    TeamcityFormatterRegistrar() {
        if (JetBrains::underTeamcity()) {
            boost::unit_test::unit_test_log.set_formatter(new JetBrains::TeamcityBoostLogFormatter());
            boost::unit_test::unit_test_log.set_threshold_level(boost::unit_test::log_successful_tests);
        }
    }
};
BOOST_GLOBAL_FIXTURE(TeamcityFormatterRegistrar);

// Formatter implementation
string toString(const_string bstr) {
    stringstream ss;
    
    ss << bstr;
    
    return ss.str();
}

TeamcityBoostLogFormatter::TeamcityBoostLogFormatter() {
}

void TeamcityBoostLogFormatter::log_start(ostream &out, counter_t test_cases_amount)
{}

void TeamcityBoostLogFormatter::log_finish(ostream &out)
{}

void TeamcityBoostLogFormatter::log_build_info(ostream &out, bool)
{}

void TeamcityBoostLogFormatter::test_unit_start(ostream &out, test_unit const& tu) {
    messages.setOutput(out);

    if (tu.p_type == TUT_CASE) {
        messages.testStarted(tu.p_name);
    } else {
        messages.suiteStarted(tu.p_name);
    }
    
    currentDetails.clear();
}

void TeamcityBoostLogFormatter::test_unit_finish(ostream &out, test_unit const& tu, unsigned long elapsed) {
    messages.setOutput(out);

    test_results const& tr = results_collector.results( tu.p_id );
    if (tu.p_type == TUT_CASE) {
        if(!tr.passed()) {
            if(tr.p_skipped) {
                messages.testIgnored(tu.p_name);
            } else if (tr.p_aborted) {
                messages.testFailed(tu.p_name, "aborted", currentDetails);
            } else {
                messages.testFailed(tu.p_name, "failed", currentDetails);
            }
        }
        
        messages.testFinished(tu.p_name);
    } else {
        messages.suiteFinished(tu.p_name);
    }
}

void TeamcityBoostLogFormatter::test_unit_skipped(ostream &out, test_unit const& tu)
{}

void TeamcityBoostLogFormatter::log_exception_start(ostream &out, log_checkpoint_data const&, boost::execution_exception const& ex) {
    string what = ex.what().begin() ? std::string(ex.what().begin(), ex.what().size()) : std::string();

    out << what << endl;
    currentDetails += what + "\n";
}

void TeamcityBoostLogFormatter::log_exception_finish(ostream &)
{}

void TeamcityBoostLogFormatter::log_entry_start(ostream&, log_entry_data const&, log_entry_types let)
{}

void TeamcityBoostLogFormatter::log_entry_value(ostream &out, const_string value) {
    out << value;
    currentDetails += toString(value);
}

void TeamcityBoostLogFormatter::log_entry_finish(ostream &out) {
    out << endl;
    currentDetails += "\n";
}

void TeamcityBoostLogFormatter::entry_context_start(ostream&, log_level)
{}

void TeamcityBoostLogFormatter::log_entry_context(ostream&, log_level, const_string)
{}

void TeamcityBoostLogFormatter::entry_context_finish(ostream&, log_level)
{}

}

#include "g13.h"
#include <fstream>

#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/support/date_time.hpp>

using namespace std;

namespace G13 {


void G13_Manager::set_log_level( ::boost::log::trivial::severity_level lvl ) {
    boost::log::core::get()->set_filter
    (
    		::boost::log::trivial::severity >= lvl
    );
    G13_OUT( "set log level to " << lvl );
}

void G13_Manager::set_log_level( const std::string &level ) {

	#define CHECK_LEVEL( L ) 												\
		if( level == BOOST_PP_STRINGIZE(L) ) {								\
			set_log_level( ::boost::log::trivial::L );						\
			return;															\
		}																	\

	CHECK_LEVEL( trace );
	CHECK_LEVEL( debug );
	CHECK_LEVEL( info );
	CHECK_LEVEL( warning );
	CHECK_LEVEL( error );
	CHECK_LEVEL( fatal );

	G13_LOG( error, "unknown log level" << level );
}

} // namespace G13



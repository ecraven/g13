#include <fstream>
#include <boost/log/attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/support/date_time.hpp>
#include "g13logging.h"

namespace expr = boost::log::expressions;

static const char* param = "--log-settings=";

g13logging::g13logging(int argc, char* argv[])
{
  init(argc, argv);
}

g13logging::~g13logging()
{
  BOOST_LOG_TRIVIAL(debug) << "g13logging dtor";
}

void
g13logging::init()
{
  boost::log::add_console_log(
    std::clog,
    boost::log::keywords::filter = boost::log::trivial::severity > boost::log::trivial::debug,
    boost::log::keywords::format = (
        expr::stream << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S") << ": "
	             << boost::log::trivial::severity << ", " << expr::smessage
	)
    );

  register_attributes();
}

void
g13logging::init(const std::string& filename)
{
  std::ifstream s(filename.c_str());

  boost::log::init_from_stream(s);

  register_attributes();
}

void
g13logging::register_attributes()
{
  boost::log::add_common_attributes(); // add LineID and TimeStamp
  boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
}

int
g13logging::init(int argc, char* argv[])
{
  for (int i=1; i<argc; i++) {
    if (std::strncmp(argv[i], param, strlen(param)) == 0) {
      const std::string filename(argv[i]+strlen(param));

      init(filename);

      BOOST_LOG_TRIVIAL(info) << "Logging initialised from " << filename;

      return 0;
    }
  }

  init();

  BOOST_LOG_TRIVIAL(info) << "Logging initialised (default configuration)";

  return 0;
}

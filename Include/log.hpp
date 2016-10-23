#ifndef _UNO_LOG_HPP_
#define _UNO_LOG_HPP_
#include "defines.h"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

NAMESPACE_PROLOG

#define SetLogSeverity(level) boost::log::core::get()->set_filter( \
	boost::log::trivial::severity >= (level))
#define LOG(severity) BOOST_LOG_TRIVIAL(severity)

NAMESPACE_EPILOG

#endif
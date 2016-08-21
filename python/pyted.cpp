#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <util/exceptions.h>
#include "PyTed.h"
#include "logging.h"

template <typename Map, typename K, typename V>
const V& genericGetter(const Map& map, const K& k) { return map[k]; }
template <typename Map, typename K, typename V>
void genericSetter(Map& map, const K& k, const V& value) { map[k] = value; }

// iterator traits specializations
//
// if clang
//   if clang < 6
//      boost::detail
//   else
//      std
// else if gcc
//   if gcc < 5
//      boost::detail
//   else
//      std
// else
//   std
#if (defined __clang__ && __clang_major__ < 6) || (defined __GNUC__ && __GNUC__ < 5)
namespace boost { namespace detail {
#else
namespace std {
#endif

#if (defined __clang__ && __clang_major__ < 6) || (defined __GNUC__ && __GNUC__ < 5)
}} // namespace boost::detail
#else
} // namespace std
#endif

#if defined __clang__ && __clang_major__ < 6
// std::shared_ptr support
	template<class T> T* get_pointer(std::shared_ptr<T> p){ return p.get(); }
#endif

namespace pyted {

/**
 * Translates an Exception into a python exception.
 *
 **/
void translateException(const Exception& e) {

	if (boost::get_error_info<error_message>(e))
		PyErr_SetString(PyExc_RuntimeError, boost::get_error_info<error_message>(e)->c_str());
	else
		PyErr_SetString(PyExc_RuntimeError, e.what());
}

/**
 * Defines all the python classes in the module libpyted. Here we decide 
 * which functions and data members we wish to expose.
 */
BOOST_PYTHON_MODULE(pyted) {

	boost::python::register_exception_translator<Exception>(&translateException);

	// Logging
	boost::python::enum_<logger::LogLevel>("LogLevel")
			.value("Quiet", logger::Quiet)
			.value("Error", logger::Error)
			.value("Debug", logger::Debug)
			.value("All", logger::All)
			.value("User", logger::User)
			;
	boost::python::def("setLogLevel", setLogLevel);
	boost::python::def("getLogLevel", getLogLevel);

	boost::python::numeric::array::set_module_and_type("numpy", "ndarray");

	boost::python::class_<PyTed>("Ted")
			.def("report_ted", &PyTed::reportTed)
			.def("report_rand", &PyTed::reportRand)
			.def("report_voi", &PyTed::reportVoi)
			.def("create_report", &PyTed::createReport)
			;
}

} // namespace pyted

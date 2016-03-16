#ifndef SQLITE3PP_CHECKED_STATEMENT_HPP
#define SQLITE3PP_CHECKED_STATEMENT_HPP

#include <silicium/identity.hpp>
#include <sqlite3pp/statement.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>

namespace sqlite3pp
{
	namespace detail
	{
		inline Si::optional<sqlite3_int64> column(sqlite3_stmt &statement, positive_int index,
		                                          Si::identity<sqlite3_int64>)
		{
			return column_int64(statement, index);
		}

		inline Si::optional<double> column(sqlite3_stmt &statement, positive_int index, Si::identity<double>)
		{
			return column_double(statement, index);
		}

		inline Si::optional<Si::memory_range> column(sqlite3_stmt &statement, positive_int index,
		                                             Si::identity<Si::memory_range>)
		{
			return column_text(statement, index);
		}
	}

	template <class BoundArguments, class ResultColumns>
	struct checked_statement
	{
		typedef Si::bounded_int<int, 0, boost::mpl::size<BoundArguments>::value - 1> argument_index;
		typedef Si::bounded_int<int, 0, boost::mpl::size<ResultColumns>::value - 1> column_index;

		statement_handle statement;

		explicit checked_statement(statement_handle statement)
		    : statement(std::move(statement))
		{
		}

		template <int Index>
		boost::system::error_code
		bind(typename boost::mpl::at<BoundArguments, boost::mpl::int_<Index>>::type const &value)
		{
			return sqlite3pp::bind(*statement, Si::literal<int, Index>(), value);
		}

		template <int Index>
		Si::optional<typename boost::mpl::at<ResultColumns, boost::mpl::int_<Index>>::type> column()
		{
			return detail::column(
			    *statement, Si::literal<int, Index>(),
			    Si::identity<typename boost::mpl::at<ResultColumns, boost::mpl::int_<Index>>::type>());
		}
	};

	template <class BoundArguments, class ResultColumns>
	Si::error_or<checked_statement<BoundArguments, ResultColumns>> prepare_checked(sqlite3 &database,
	                                                                               Si::c_string query)
	{
		return Si::map(prepare(database, query), [](statement_handle statement)
		               {
			               return checked_statement<BoundArguments, ResultColumns>(std::move(statement));
			           });
	}
}

#endif

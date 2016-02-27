#ifndef SQLITE3PP_CHECKED_STATEMENT_HPP
#define SQLITE3PP_CHECKED_STATEMENT_HPP

#include <sqlite3pp/statement.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>

namespace sqlite3pp
{
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

		sqlite3_int64 column_int64(column_index column)
		{
			return sqlite3pp::column_int64(*statement, column);
		}

		double column_double(column_index column)
		{
			return sqlite3pp::column_double(*statement, column);
		}

		Si::memory_range column_text(column_index column)
		{
			return sqlite3pp::column_text(*statement, column);
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

#ifndef SQLITE3PP_CHECKED_STATEMENT_HPP
#define SQLITE3PP_CHECKED_STATEMENT_HPP

#include <sqlite3pp/statement.hpp>

namespace sqlite3pp
{
	template <std::uint16_t BoundArguments, std::uint16_t ResultColumns>
	struct checked_statement
	{
		typedef Si::bounded_int<int, 0, BoundArguments - 1> argument_index;
		typedef Si::bounded_int<int, 0, ResultColumns - 1> column_index;

		statement_handle statement;

		explicit checked_statement(statement_handle statement)
		    : statement(std::move(statement))
		{
		}

		template <class Value>
		boost::system::error_code bind(argument_index argument, Value const &value)
		{
			return sqlite3pp::bind(*statement, argument, value);
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

	template <std::uint16_t BoundArguments, std::uint16_t ResultColumns>
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

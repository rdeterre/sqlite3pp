#ifndef SQLITE3PP_STATEMENT_HPP
#define SQLITE3PP_STATEMENT_HPP

#include <sqlite3pp/sqlite3.hpp>

namespace sqlite3pp
{
	struct statement_deleter
	{
		void operator()(sqlite3_stmt *statement) const
		{
			assert(statement);
			sqlite3_finalize(statement);
		}
	};

	typedef std::unique_ptr<sqlite3_stmt, statement_deleter> statement_handle;

	inline Si::error_or<statement_handle> prepare(sqlite3 &database, Si::c_string query)
	{
		sqlite3_stmt *statement;
		int rc = sqlite3_prepare_v2(&database, query.c_str(), -1, &statement, nullptr);
		if (rc != SQLITE_OK)
		{
			return make_error_code(rc);
		}
		return statement_handle(statement);
	}

	typedef Si::bounded_int<int, 0,
#if SILICIUM_COMPILER_HAS_CONSTEXPR_NUMERIC_LIMITS
	                        (std::numeric_limits<int>::max)()
#else
	                        INT_MAX
#endif
	                        > positive_int;

	inline boost::system::error_code bind(sqlite3_stmt &statement, positive_int zero_based_index, sqlite3_int64 value)
	{
		return make_error_code(sqlite3_bind_int64(&statement, zero_based_index.value() + 1, value));
	}

	inline boost::system::error_code bind(sqlite3_stmt &statement, positive_int zero_based_index, double value)
	{
		return make_error_code(sqlite3_bind_double(&statement, zero_based_index.value() + 1, value));
	}

	typedef Si::bounded_int<int, 0,
#if SILICIUM_COMPILER_HAS_CONSTEXPR_NUMERIC_LIMITS
	                        (std::numeric_limits<int>::max)()
#else
	                        INT_MAX
#endif
	                        > text_length;

	typedef Si::array_view<char const, text_length> text_view;

	inline boost::system::error_code bind(sqlite3_stmt &statement, positive_int zero_based_index, text_view data)
	{
		char const *const ptr = data.empty() ? "" : data.begin();
		return make_error_code(
		    sqlite3_bind_text(&statement, zero_based_index.value() + 1, ptr, data.length().value(), nullptr));
	}

	enum class step_result
	{
		row = SQLITE_ROW,
		done = SQLITE_DONE
	};

	inline std::ostream &operator<<(std::ostream &out, step_result result)
	{
		return out << static_cast<int>(result);
	}

	inline Si::error_or<step_result> step(sqlite3_stmt &statement)
	{
		int rc = sqlite3_step(&statement);
		switch (rc)
		{
		case SQLITE_ROW:
			return step_result::row;
		case SQLITE_DONE:
			return step_result::done;
		default:
			return make_error_code(rc);
		}
	}

	inline positive_int column_count(sqlite3_stmt &statement)
	{
		return *positive_int::create(sqlite3_column_count(&statement));
	}

	namespace detail
	{
		enum class type
		{
			integer = SQLITE_INTEGER,
			float_ = SQLITE_FLOAT,
			blob = SQLITE_BLOB,
			null = SQLITE_NULL,
			text = SQLITE_TEXT
		};

		inline type column_type(sqlite3_stmt &statement, positive_int zero_based_index)
		{
			assert(zero_based_index < column_count(statement));
			return static_cast<type>(sqlite3_column_type(&statement, zero_based_index.value()));
		}
	}

	inline Si::optional<sqlite3_int64> column_int64(sqlite3_stmt &statement, positive_int zero_based_index)
	{
		assert(zero_based_index < column_count(statement));
		switch (detail::column_type(statement, zero_based_index))
		{
		case detail::type::integer:
			return sqlite3_column_int64(&statement, zero_based_index.value());

		case detail::type::float_:
		case detail::type::blob:
		case detail::type::null:
		case detail::type::text:
			return Si::none;
		}
		SILICIUM_UNREACHABLE();
	}

	inline Si::optional<double> column_double(sqlite3_stmt &statement, positive_int zero_based_index)
	{
		assert(zero_based_index < column_count(statement));
		switch (detail::column_type(statement, zero_based_index))
		{
		case detail::type::float_:
			return sqlite3_column_double(&statement, zero_based_index.value());

		case detail::type::integer:
		case detail::type::blob:
		case detail::type::null:
		case detail::type::text:
			return Si::none;
		}
		SILICIUM_UNREACHABLE();
	}

	inline Si::optional<Si::memory_range> column_text(sqlite3_stmt &statement, positive_int zero_based_index)
	{
		assert(zero_based_index < column_count(statement));
		switch (detail::column_type(statement, zero_based_index))
		{
		case detail::type::text:
		{
			char const *data =
			    reinterpret_cast<char const *>(sqlite3_column_text(&statement, zero_based_index.value()));
			int length = sqlite3_column_bytes(&statement, zero_based_index.value());
			return Si::memory_range(data, data + length);
		}

		case detail::type::integer:
		case detail::type::float_:
		case detail::type::blob:
		case detail::type::null:
			return Si::none;
		}
		SILICIUM_UNREACHABLE();
	}

	template <class Action>
	auto begin_transaction(sqlite3 &database, Action &&transaction_content)
	    -> decltype(std::forward<Action>(transaction_content)())
	{
		step(*prepare(database, "BEGIN").move_value()).move_value();
		Si::optional<decltype(std::forward<Action>(transaction_content)())> result;
#if SILICIUM_HAS_EXCEPTIONS
		try
#endif
		{
			result = std::forward<Action>(transaction_content)();
		}
#if SILICIUM_HAS_EXCEPTIONS
		catch (...)
		{
			step(*prepare(database, "ROLLBACK").move_value()).move_value();
			throw;
		}
#endif
		step(*prepare(database, "COMMIT").move_value()).move_value();
		return *std::move(result);
	}
}

#endif

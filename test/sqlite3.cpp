#include <sqlite3pp/checked_statement.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/equal.hpp>

#if SILICIUM_HAS_EXCEPTIONS
#include <boost/filesystem/operations.hpp>

BOOST_AUTO_TEST_CASE(sqlite_open_or_create_file)
{
	boost::filesystem::path const test_file = boost::filesystem::current_path() / "test_sqlite_open_file.sqlite3pp";
	boost::filesystem::remove(test_file);
	{
		Si::error_or<sqlite3pp::database_handle> database =
		    sqlite3pp::open_or_create(Si::c_string(test_file.string().c_str()));
		BOOST_REQUIRE(!database.is_error());
		BOOST_CHECK(database.get());
		sqlite3pp::database_handle moved = database.move_value();
		BOOST_CHECK(moved);
	}
	// reopen an existing file:
	{
		Si::error_or<sqlite3pp::database_handle> database =
		    sqlite3pp::open_or_create(Si::c_string(test_file.string().c_str()));
		BOOST_REQUIRE(!database.is_error());
		BOOST_CHECK(database.get());
		sqlite3pp::database_handle moved = database.move_value();
		BOOST_CHECK(moved);
	}
}
#endif

BOOST_AUTO_TEST_CASE(sqlite_open_existing_memory)
{
	Si::error_or<sqlite3pp::database_handle> database = sqlite3pp::open_existing(":memory:");
	BOOST_REQUIRE(!database.is_error());
	BOOST_CHECK(database.get());
	sqlite3pp::database_handle moved = database.move_value();
	BOOST_CHECK(moved);
}

BOOST_AUTO_TEST_CASE(sqlite_open_or_create_memory)
{
	Si::error_or<sqlite3pp::database_handle> database = sqlite3pp::open_or_create(":memory:");
	BOOST_REQUIRE(!database.is_error());
	BOOST_CHECK(database.get());
	sqlite3pp::database_handle moved = database.move_value();
	BOOST_CHECK(moved);
}

BOOST_AUTO_TEST_CASE(sqlite_open_error)
{
	Si::error_or<sqlite3pp::database_handle> database = sqlite3pp::open_existing("/");
	BOOST_CHECK(database.is_error());
}

BOOST_AUTO_TEST_CASE(sqlite_prepare)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	Si::error_or<sqlite3pp::statement_handle> statement = sqlite3pp::prepare(*database, "SELECT 1");
	BOOST_REQUIRE(!statement.is_error());
	BOOST_CHECK(statement.get());
	sqlite3pp::statement_handle moved = statement.move_value();
	BOOST_CHECK(moved);
}

BOOST_AUTO_TEST_CASE(sqlite_bind_int64)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT ?").move_value();
	BOOST_CHECK(!sqlite3pp::bind(*statement, Si::literal<int, 0>(), static_cast<sqlite3_int64>(123456)));
}

BOOST_AUTO_TEST_CASE(sqlite_bind_text)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT ?").move_value();
	Si::memory_range const expected = Si::make_c_str_range("abc");
	BOOST_CHECK(!sqlite3pp::bind(
	    *statement, Si::literal<int, 0>(),
	    sqlite3pp::text_view(*expected.begin(), sqlite3pp::text_length::create(static_cast<int>(expected.size()))
	                                                .or_throw([]
	                                                          {
		                                                          BOOST_FAIL("cannot fail");
		                                                      }))));
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL((Si::literal<int, 1>()), sqlite3pp::column_count(*statement));
	Si::memory_range const got = sqlite3pp::column_text(*statement, Si::literal<int, 0>());
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), got.begin(), got.end());
}

BOOST_AUTO_TEST_CASE(sqlite_step)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT 1").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL((Si::literal<int, 1>()), sqlite3pp::column_count(*statement));
	BOOST_CHECK_EQUAL(1, sqlite3pp::column_int64(*statement, Si::literal<int, 0>()));
	BOOST_CHECK_EQUAL(sqlite3pp::step_result::done, sqlite3pp::step(*statement).get());
}

BOOST_AUTO_TEST_CASE(sqlite_column_double)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT 1.5").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL((Si::literal<int, 1>()), sqlite3pp::column_count(*statement));
	BOOST_CHECK_EQUAL(1.5, sqlite3pp::column_double(*statement, Si::literal<int, 0>()));
}

BOOST_AUTO_TEST_CASE(sqlite_column_text)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT \"abc\"").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL((Si::literal<int, 1>()), sqlite3pp::column_count(*statement));
	Si::memory_range const expected = Si::make_c_str_range("abc");
	Si::memory_range const got = sqlite3pp::column_text(*statement, Si::literal<int, 0>());
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), got.begin(), got.end());
}

BOOST_AUTO_TEST_CASE(begin_transaction_commit)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::step(*sqlite3pp::prepare(*database, "CREATE TABLE \"test\" (\"field\" INTEGER NOT NULL)").move_value())
	    .move_value();
	BOOST_CHECK_EQUAL(
	    456, sqlite3pp::begin_transaction(
	             *database, [&database]()
	             {
		             sqlite3pp::step(
		                 *sqlite3pp::prepare(*database, "INSERT INTO \"test\" (\"field\") VALUES (123)").move_value())
		                 .move_value();
		             return 456;
		         }));
	sqlite3pp::statement_handle const select =
	    sqlite3pp::prepare(*database, "SELECT COUNT(*) FROM \"test\"").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*select).move_value());
	BOOST_CHECK_EQUAL(1, sqlite3pp::column_int64(*select, Si::literal<int, 0>()));
}

#if SILICIUM_HAS_EXCEPTIONS
BOOST_AUTO_TEST_CASE(begin_transaction_throw)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::step(*sqlite3pp::prepare(*database, "CREATE TABLE \"test\" (\"field\" INTEGER NOT NULL)").move_value())
	    .move_value();
	BOOST_CHECK_EXCEPTION(
	    sqlite3pp::begin_transaction(
	        *database,
	        [&database]() -> int
	        {
		        sqlite3pp::step(
		            *sqlite3pp::prepare(*database, "INSERT INTO \"test\" (\"field\") VALUES (123)").move_value())
		            .move_value();
		        throw std::runtime_error("rollback");
		    }),
	    std::runtime_error, [](std::runtime_error const &ex)
	    {
		    return ex.what() == std::string("rollback");
		});
	sqlite3pp::statement_handle const select =
	    sqlite3pp::prepare(*database, "SELECT COUNT(*) FROM \"test\"").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*select).move_value());
	BOOST_CHECK_EQUAL(0, sqlite3pp::column_int64(*select, Si::literal<int, 0>()));
}
#endif

BOOST_AUTO_TEST_CASE(sqlite_checked_statement_handle)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	typedef boost::mpl::vector<sqlite3_int64, double, sqlite3pp::text_view> bound;
	typedef boost::mpl::vector<sqlite3_int64, double, Si::memory_range, sqlite3_int64> columns;
	sqlite3pp::checked_statement<bound, columns> statement =
	    sqlite3pp::prepare_checked<bound, columns>(*database, "SELECT ?, ?, ?, -3").move_value();
	statement.bind<0>(static_cast<sqlite3_int64>(123));
	statement.bind<1>(456.0);
	statement.bind<2>(sqlite3pp::text_view(*"abc", Si::literal<int, 3>()));
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement.statement).get());
	BOOST_REQUIRE_EQUAL((Si::literal<int, 4>()), sqlite3pp::column_count(*statement.statement));
	BOOST_CHECK_EQUAL(123, statement.column<0>());
	BOOST_CHECK_EQUAL(456.0, statement.column<1>());
	BOOST_CHECK(boost::range::equal(Si::make_c_str_range("abc"), statement.column<2>()));
	BOOST_CHECK_EQUAL(-3, statement.column<3>());
	BOOST_CHECK_EQUAL(sqlite3pp::step_result::done, sqlite3pp::step(*statement.statement).get());
}

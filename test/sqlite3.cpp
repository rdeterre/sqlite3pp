#include <sqlite3pp/sqlite3.hpp>
#include <boost/test/unit_test.hpp>

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
	BOOST_CHECK(!sqlite3pp::bind(*statement, sqlite3pp::positive_int::literal<0>(), 123456));
}

BOOST_AUTO_TEST_CASE(sqlite_bind_text)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT ?").move_value();
	Si::memory_range const expected = Si::make_c_str_range("abc");
	BOOST_CHECK(!sqlite3pp::bind(*statement, sqlite3pp::positive_int::literal<0>(), *expected.begin(),
	                             static_cast<int>(expected.size())));
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL(sqlite3pp::positive_int::literal<1>(), sqlite3pp::column_count(*statement));
	Si::memory_range const got = sqlite3pp::column_text(*statement, sqlite3pp::positive_int::literal<0>());
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), got.begin(), got.end());
}

BOOST_AUTO_TEST_CASE(sqlite_step)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT 1").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL(sqlite3pp::positive_int::literal<1>(), sqlite3pp::column_count(*statement));
	BOOST_CHECK_EQUAL(1, sqlite3pp::column_int64(*statement, sqlite3pp::positive_int::literal<0>()));
	BOOST_CHECK_EQUAL(sqlite3pp::step_result::done, sqlite3pp::step(*statement).get());
}

BOOST_AUTO_TEST_CASE(sqlite_column_double)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT 1.5").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL(sqlite3pp::positive_int::literal<1>(), sqlite3pp::column_count(*statement));
	BOOST_CHECK_EQUAL(1.5, sqlite3pp::column_double(*statement, sqlite3pp::positive_int::literal<0>()));
}

BOOST_AUTO_TEST_CASE(sqlite_column_text)
{
	sqlite3pp::database_handle database = sqlite3pp::open_existing(":memory:").move_value();
	sqlite3pp::statement_handle statement = sqlite3pp::prepare(*database, "SELECT \"abc\"").move_value();
	BOOST_REQUIRE_EQUAL(sqlite3pp::step_result::row, sqlite3pp::step(*statement).get());
	BOOST_REQUIRE_EQUAL(sqlite3pp::positive_int::literal<1>(), sqlite3pp::column_count(*statement));
	Si::memory_range const expected = Si::make_c_str_range("abc");
	Si::memory_range const got = sqlite3pp::column_text(*statement, sqlite3pp::positive_int::literal<0>());
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
	BOOST_CHECK_EQUAL(1, sqlite3pp::column_int64(*select, sqlite3pp::positive_int::literal<0>()));
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
	BOOST_CHECK_EQUAL(0, sqlite3pp::column_int64(*select, sqlite3pp::positive_int::literal<0>()));
}
#endif

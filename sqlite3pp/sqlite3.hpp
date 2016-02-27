#ifndef SQLITE3PP_SQLITE3_HPP
#define SQLITE3PP_SQLITE3_HPP

#include <silicium/c_string.hpp>
#include <silicium/memory_range.hpp>
#include <silicium/error_or.hpp>
#include <silicium/array_view.hpp>
#include <silicium/bounded_int.hpp>
#include <sqlite3.h>
#include <memory>

namespace sqlite3pp
{
	struct database_deleter
	{
		void operator()(sqlite3 *database) const
		{
			assert(database);
			sqlite3_close(database);
		}
	};

	typedef std::unique_ptr<sqlite3, database_deleter> database_handle;

	struct error_category : boost::system::error_category
	{
		virtual const char *name() const BOOST_NOEXCEPT SILICIUM_OVERRIDE
		{
			return "sqlite3";
		}

		virtual std::string message(int ev) const SILICIUM_OVERRIDE
		{
#if SQLITE_VERSION_NUMBER >= 3007015
			// http://www.sqlite.org/changes.html
			return sqlite3_errstr(ev);
#else
			(void)ev;
			return "?";
#endif
		}
	};

	inline boost::system::error_category &sqlite_category()
	{
		static error_category instance;
		return instance;
	}

	inline boost::system::error_code make_error_code(int code)
	{
		return boost::system::error_code(code, sqlite_category());
	}

	inline Si::error_or<database_handle> open_existing(Si::c_string path)
	{
		sqlite3 *database;
		int rc = sqlite3_open_v2(path.c_str(), &database, SQLITE_OPEN_READWRITE, nullptr);
		if (rc != SQLITE_OK)
		{
			return make_error_code(rc);
		}
		return database_handle(database);
	}

	inline Si::error_or<database_handle> open_or_create(Si::c_string path)
	{
		sqlite3 *database;
		int rc = sqlite3_open_v2(path.c_str(), &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
		if (rc != SQLITE_OK)
		{
			return make_error_code(rc);
		}
		return database_handle(database);
	}
}

#endif

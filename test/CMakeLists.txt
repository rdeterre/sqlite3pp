#Boost.Test uses typeid for no reason
if(NOT SQLITE3PP_NO_RTTI)
	include_directories(.)
	file(GLOB sources "*.hpp" "*.cpp")
	file(GLOB_RECURSE headers "../sqlite3pp/*.hpp")
	set(allSources ${sources} ${headers})
	set(formatted ${formatted} ${allSources} PARENT_SCOPE)
	add_executable(tests ${allSources})
	target_link_libraries(tests ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CONAN_LIBS})
	if(SQLITE3PP_LINUX)
		target_link_libraries(tests dl)
	endif()
endif()

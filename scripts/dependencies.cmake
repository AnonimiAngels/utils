set(LIBRARIES_DIR ${PROJECT_DIR}/../libraries)
set(LIBRARIES_BIN_DIR ${CMAKE_BINARY_DIR}/libraries)

message(STATUS "Libraries directory: ${LIBRARIES_DIR}")

function(proccess_deps OUT_LIBS OUT_INCS)
	# Initialize variables in the parent scope
	SET(L_LIBS "" PARENT_SCOPE)
	SET(L_INCS "" PARENT_SCOPE)

	CPMAddPackage(
		NAME spdlog
		GITHUB_REPOSITORY gabime/spdlog
		GIT_TAG v1.15.1
	)
	LIST(APPEND L_LIBS spdlog::spdlog_header_only)

	CPMAddPackage(
		NAME fmt
		GITHUB_REPOSITORY fmtlib/fmt
		GIT_TAG 11.1.4
	)
	LIST(APPEND L_LIBS fmt)

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(LIBSSH2 REQUIRED libssh2)

	LIST(APPEND L_LIBS ${LIBSSH2_LIBRARIES})

	# Ensure variables are set in the parent scope
	SET(${OUT_LIBS} ${L_LIBS} PARENT_SCOPE)
	SET(${OUT_INCS} ${L_INCS} PARENT_SCOPE)
endfunction()

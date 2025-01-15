set(LIBRARIES_DIR ${PROJECT_DIR}/../libraries)
set(LIBRARIES_BIN_DIR ${CMAKE_BINARY_DIR}/libraries)

message(STATUS "Libraries directory: ${LIBRARIES_DIR}")

function(proccess_deps OUT_LIBS OUT_INCS)
	# Initialize variables in the parent scope
	set(L_LIBS "" PARENT_SCOPE)
	set(L_INCS "" PARENT_SCOPE)

	CPMAddPackage(
		NAME spdlog
		GITHUB_REPOSITORY gabime/spdlog
		VERSION 1.9.2
	)
	list(APPEND L_LIBS spdlog)

	# Ensure variables are set in the parent scope
	set(${OUT_LIBS} ${L_LIBS} PARENT_SCOPE)
	set(${OUT_INCS} ${L_INCS} PARENT_SCOPE)
endfunction()

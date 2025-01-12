find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(VIRTUAL_ENV "${PROJECT_SOURCE_DIR}/.venv")
set(VIRTUAL_ENV_BIN "${VIRTUAL_ENV}/bin")

if(NOT EXISTS "${VIRTUAL_ENV}")
  execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "${VIRTUAL_ENV}"
                          COMMAND_ERROR_IS_FATAL ANY)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit")
  execute_process(COMMAND "${VIRTUAL_ENV_BIN}/pip" install pre-commit
                          COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND "${VIRTUAL_ENV_BIN}/pre-commit" install
                          COMMAND_ERROR_IS_FATAL ANY)
endif()

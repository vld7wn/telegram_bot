cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0167 OLD)

# Название проекта
project(my_telegram_bot CXX)

if(MSVC)
    add_compile_options(/FS)
endif()

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Устанавливаем версию Windows 10 для поддержки cpp-httplib
add_compile_definitions(_WIN32_WINNT=0x0A00)
add_compile_definitions(WINVER=0x0A00)

# --- ПОИСК ЗАВИСИМОСТЕЙ (VCPKG) ---
# Toolchain передаётся через -DCMAKE_TOOLCHAIN_FILE или автоматически через VCPKG_ROOT
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

find_package(Boost REQUIRED system)
find_package(xlnt QUIET)
if(xlnt_FOUND)
    message(STATUS "xlnt found - Excel export enabled")
    add_definitions(-DHAS_XLNT)
else()
    message(STATUS "xlnt not found - Excel export disabled")
endif()
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# find_package(TgBot) не сработал, ищем библиотеку вручную
find_library(TGBOT_LIB NAMES tgbot TgBot tgbot-cpp PATHS 
    "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib"
    "D:/vcpkg/installed/x64-windows/lib"
)

if(NOT TGBOT_LIB)
    message(FATAL_ERROR "TgBot library not found!")
else()
    message(STATUS "Found TgBot library: ${TGBOT_LIB}")
endif()

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# --- ОПРЕДЕЛЕНИЕ БИБЛИОТЕКИ ЛОГИКИ ---
add_library(bot_logic
		admin_panel.cpp
		application_flow.cpp
		config.cpp
		database.cpp
		excel_generate.cpp
		faq_manager.cpp
		handler_registration.cpp
		http_server.cpp
		logger.cpp
		message_to_client.cpp
		session_manager.cpp
		state_handler.cpp
		super_admin.cpp
		tariff_manager.cpp
		trade_points.cpp
		utils.cpp
)

# --- ДОБАВЛЕНИЕ ПАПОК С ЗАГОЛОВОЧНЫМИ ФАЙЛАМИ К БИБЛИОТЕКЕ ---
target_include_directories(bot_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Добавляем пути к заголовкам VCPKG если переменная окружения задана
if(DEFINED ENV{VCPKG_ROOT})
    target_include_directories(bot_logic PUBLIC "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
    link_directories("$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
else()
    target_include_directories(bot_logic PUBLIC "D:/vcpkg/installed/x64-windows/include")
    link_directories("D:/vcpkg/installed/x64-windows/lib")
endif()

# --- ПОДКЛЮЧЕНИЕ БИБЛИОТЕК (ЛИНКОВКА) К БИБЛИОТЕКЕ ---

target_link_libraries(bot_logic PRIVATE
		# Линкуемся с найденной библиотекой TgBot
		${TGBOT_LIB}

		# Остальные библиотеки, найденные через find_package
		unofficial::sqlite3::sqlite3
		nlohmann_json::nlohmann_json
		Boost::system
		
		# OpenSSL и CURL (уже включены в TgBot::TgBot, но можно оставить явно)
		OpenSSL::SSL
		OpenSSL::Crypto
		CURL::libcurl
)

# Условная линковка xlnt если найдена
if(xlnt_FOUND)
    target_link_libraries(bot_logic PRIVATE xlnt::xlnt)
endif()

# Системные библиотеки Windows для сети
if(WIN32)
    target_link_libraries(bot_logic PRIVATE
        ws2_32
        wsock32
        crypt32
    )
endif()

# --- ОПРЕДЕЛЕНИЕ ИСПОЛНЯЕМОГО ФАЙЛА ---
add_executable(my_telegram_bot main.cpp)

# Линкуем исполняемый файл с нашей библиотекой логики.
# Он автоматически унаследует все PUBLIC зависимости и пути от bot_logic.
target_link_libraries(my_telegram_bot PRIVATE bot_logic)

# --- КОПИРОВАНИЕ JSON ФАЙЛОВ В ПАПКУ СБОРКИ ---
file(GLOB_RECURSE JSON_FILES "json-cfg/*.json")
add_custom_command(
		TARGET my_telegram_bot POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:my_telegram_bot>/json-cfg
)
foreach(JSON_FILE ${JSON_FILES})
	get_filename_component(JSON_NAME ${JSON_FILE} NAME)
	add_custom_command(
			TARGET my_telegram_bot POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			${JSON_FILE}
			$<TARGET_FILE_DIR:my_telegram_bot>/json-cfg/${JSON_NAME}
	)
endforeach()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0167 OLD)

# Название проекта
project(my_telegram_bot CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ПОИСК ЗАВИСИМОСТЕЙ (VCPKG) ---
# Toolchain передаётся через -DCMAKE_TOOLCHAIN_FILE или автоматически через VCPKG_ROOT
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

find_package(Boost REQUIRED system)
find_package(xlnt REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# --- ОПРЕДЕЛЕНИЕ БИБЛИОТЕКИ ЛОГИКИ ---
add_library(bot_logic
		admin_panel.cpp
		application_flow.cpp
		config.cpp
		database.cpp
		excel_generate.cpp
		faq_manager.cpp
		handler_registration.cpp
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

# Определяем triplet если не задан
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
endif()

# Добавляем путь к заголовочным файлам vcpkg (для tgbot-cpp и других библиотек)
if(DEFINED ENV{VCPKG_ROOT})
    target_include_directories(bot_logic PUBLIC "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
    link_directories("$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
else()
    # Fallback для систем без VCPKG_ROOT
    target_include_directories(bot_logic PUBLIC "D:/vcpkg/installed/x64-mingw-static/include")
    link_directories("D:/vcpkg/installed/x64-mingw-static/lib")
endif()

# --- ПОДКЛЮЧЕНИЕ БИБЛИОТЕК (ЛИНКОВКА) К БИБЛИОТЕКЕ ---

# Находим OpenSSL
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

target_link_libraries(bot_logic PRIVATE
		# Линкуемся с tgbot-cpp напрямую по имени.
		TgBot

		# Остальные библиотеки, найденные через find_package
		xlnt::xlnt
		unofficial::sqlite3::sqlite3
		nlohmann_json::nlohmann_json
		Boost::system
		
		# OpenSSL и CURL для TgBot
		OpenSSL::SSL
		OpenSSL::Crypto
		CURL::libcurl
		
		# Системные библиотеки Windows для сети
		ws2_32
		wsock32
		crypt32
)

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

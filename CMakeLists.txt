cmake_minimum_required(VERSION 3.21...3.31)
project(smirkly-chat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_GENERATOR MATCHES "Ninja")
    set_property(GLOBAL PROPERTY JOB_POOLS link_pool=1)
    set(CMAKE_JOB_POOL_LINK link_pool)
endif()

option(SMIRKLY_CHAT_ENABLE_TESTSUITE "Enable userver functional tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

set(USERVER_FEATURE_POSTGRESQL ON)
set(USERVER_FEATURE_PATCH_LIBPQ OFF)
set(USERVER_FEATURE_REDIS ON)
set(USERVER_FEATURE_KAFKA ON)
set(USERVER_FEATURE_TESTSUITE ${SMIRKLY_CHAT_ENABLE_TESTSUITE})
set(USERVER_FEATURE_UTEST ${SMIRKLY_CHAT_ENABLE_TESTSUITE})

find_package(
    userver 3.0 EXACT
    COMPONENTS core kafka postgresql redis
    QUIET
)
if(NOT userver_FOUND)
    download_userver(TRY_DIR third_party/userver GIT_TAG v3.0)
endif()

find_package(OpenSSL REQUIRED Crypto)
userver_setup_environment()

set(SMIRKLY_CHAT_SOURCES
    src/chat/api/health/liveness_handler.cpp
    src/chat/api/health/readiness_handler.cpp
    src/chat/api/v0/websocket/chat_websocket_handler.cpp
    src/chat/api/v0/websocket/protocol/event_codec.cpp
    src/chat/platform/auth/jwks_verifier_component.cpp
)

add_library(smirkly-chat-objs OBJECT ${SMIRKLY_CHAT_SOURCES})
target_compile_features(smirkly-chat-objs PUBLIC cxx_std_20)
target_include_directories(smirkly-chat-objs PUBLIC src)
target_link_libraries(
    smirkly-chat-objs
    PUBLIC
        OpenSSL::Crypto
        userver::core
        userver::kafka
        userver::postgresql
        userver::redis
)

add_executable(smirkly-chat src/main.cpp)
target_compile_features(smirkly-chat PRIVATE cxx_std_20)
target_compile_definitions(
    smirkly-chat
    PRIVATE SMIRKLY_CHAT_ENABLE_TESTSUITE=$<BOOL:${SMIRKLY_CHAT_ENABLE_TESTSUITE}>
)
target_link_libraries(smirkly-chat PRIVATE smirkly-chat-objs)

if(SMIRKLY_CHAT_ENABLE_TESTSUITE)
    add_executable(
        smirkly-chat-unittest
        tests/unit/jwt_utils_test.cpp
        tests/unit/websocket_protocol_test.cpp
    )
    target_link_libraries(
        smirkly-chat-unittest
        PRIVATE smirkly-chat-objs userver::utest
    )
    add_google_tests(smirkly-chat-unittest)

    userver_testsuite_add_simple(
        SERVICE_TARGET smirkly-chat
        TEST_SUFFIX functional
        WORKING_DIRECTORY tests/functional
        REQUIREMENTS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    )
endif()

include(GNUInstallDirs)
install(
    TARGETS smirkly-chat
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT smirkly-chat
)
install(
    FILES configs/static_config.prod.yaml
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/smirkly-chat
    COMPONENT smirkly-chat
)

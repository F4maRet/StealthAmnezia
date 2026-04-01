cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT StealthAmnezia)
set(STEALTHAMNEZIA_VERSION 1.0.0)

project(${PROJECT} VERSION ${STEALTHAMNEZIA_VERSION}
        DESCRIPTION "StealthAmnezia — ультра-лёгкий необнаруживаемый VPN"
        HOMEPAGE_URL "https://github.com/F4maRet/StealthAmnezia")

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(RELEASE_DATE "${CURRENT_DATE}")

set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    if(IOS)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    elseif(MACOS_NE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_subdirectory(service)
endif()

# StealthAmnezia: убрали всё legacy (OpenVPN, Shadowsocks и т.д.)
# Только AWG + XRay + новый CPS-генератор и авто-updater
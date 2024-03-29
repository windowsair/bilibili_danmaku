﻿#----------------------------------------
# Detect and set platform variable names
#     - for non-windows build platform & architecture is detected using inbuilt CMAKE variables and cross target component configure
#     - for windows we use the passed in parameter to CMAKE to determine build arch
#----------------------------------------

if (WIN32)
    # For windows it is expected that CLR_CMAKE_HOST_ARCH parameter is passed
    # to CMAKE to determine build arch.
    # If it is not passed, detect arch here.
    if ("${CLR_CMAKE_HOST_ARCH}" STREQUAL "")
        if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            set(CLR_CMAKE_HOST_ARCH x86)
        else()
            set(CLR_CMAKE_HOST_ARCH x64)
        endif()
    endif()
endif()

# TODO: Windows ARM

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    if(CLR_CROSS_COMPONENTS_BUILD)
        # CMAKE_HOST_SYSTEM_PROCESSOR returns the value of `uname -p` on host.
        if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL amd64)
            if(CLR_CMAKE_TARGET_ARCH STREQUAL "arm")
                set(CLR_CMAKE_PLATFORM_UNIX_X86 1)
            else()
                set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
            endif()
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL i686)
            set(CLR_CMAKE_PLATFORM_UNIX_X86 1)
        else()
            clr_unknown_arch()
        endif()
    else()
        # CMAKE_SYSTEM_PROCESSOR returns the value of `uname -p` on target.
        # For the AMD/Intel 64bit architecture two different strings are common.
        # Linux and Darwin identify it as "x86_64" while FreeBSD and netbsd uses the
        # "amd64" string. Accept either of the two here.
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
            set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
            set(CLR_CMAKE_PLATFORM_UNIX_ARM 1)
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
            set(CLR_CMAKE_PLATFORM_UNIX_ARM 1)
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
            set(CLR_CMAKE_PLATFORM_UNIX_ARM64 1)
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
            set(CLR_CMAKE_PLATFORM_UNIX_X86 1)
        else()
            clr_unknown_arch()
        endif()
    endif()
    set(CLR_CMAKE_PLATFORM_LINUX 1)

    # Detect Linux ID
    if(DEFINED CLR_CMAKE_LINUX_ID)
        if(CLR_CMAKE_LINUX_ID STREQUAL ubuntu)
            set(CLR_CMAKE_TARGET_UBUNTU_LINUX 1)
        elseif(CLR_CMAKE_LINUX_ID STREQUAL tizen)
            set(CLR_CMAKE_TARGET_TIZEN_LINUX 1)
        elseif(CLR_CMAKE_LINUX_ID STREQUAL alpine)
            set(CLR_CMAKE_PLATFORM_ALPINE_LINUX 1)
        endif()
        if(CLR_CMAKE_LINUX_ID STREQUAL ubuntu)
            set(CLR_CMAKE_PLATFORM_UBUNTU_LINUX 1)
        endif()
    endif(DEFINED CLR_CMAKE_LINUX_ID)
endif(CMAKE_SYSTEM_NAME STREQUAL Linux)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
    set(CLR_CMAKE_PLATFORM_DARWIN 1)
    if(CMAKE_VERSION VERSION_LESS "3.4.0")
        set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} <FLAGS> <DEFINES> -o <OBJECT> -c <SOURCE>")
    else()
        set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} <FLAGS> <DEFINES> <INCLUDES> -o <OBJECT> -c <SOURCE>")
    endif(CMAKE_VERSION VERSION_LESS "3.4.0")
endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)

if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
    set(CLR_CMAKE_PLATFORM_FREEBSD 1)
endif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)

if(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
    set(CLR_CMAKE_PLATFORM_OPENBSD 1)
endif(CMAKE_SYSTEM_NAME STREQUAL OpenBSD)

if(CMAKE_SYSTEM_NAME STREQUAL NetBSD)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
    set(CLR_CMAKE_PLATFORM_NETBSD 1)
endif(CMAKE_SYSTEM_NAME STREQUAL NetBSD)

if(CMAKE_SYSTEM_NAME STREQUAL SunOS)
    set(CLR_CMAKE_PLATFORM_UNIX 1)
    EXECUTE_PROCESS(
            COMMAND isainfo -n
            OUTPUT_VARIABLE SUNOS_NATIVE_INSTRUCTION_SET
    )
    if(SUNOS_NATIVE_INSTRUCTION_SET MATCHES "amd64")
        set(CLR_CMAKE_PLATFORM_UNIX_AMD64 1)
        set(CMAKE_SYSTEM_PROCESSOR "amd64")
    else()
        clr_unknown_arch()
    endif()
    set(CLR_CMAKE_PLATFORM_SUNOS 1)
endif(CMAKE_SYSTEM_NAME STREQUAL SunOS)

#--------------------------------------------
# This repo builds two set of binaries
# 1. binaries which execute on target arch machine
#        - for such binaries host architecture & target architecture are same
#        - eg. coreclr.dll
# 2. binaries which execute on host machine but target another architecture
#        - host architecture is different from target architecture
#        - eg. crossgen.exe - runs on x64 machine and generates nis targeting arm64
#        - for complete list of such binaries refer to file crosscomponents.cmake
#-------------------------------------------------------------
# Set HOST architecture variables
if(CLR_CMAKE_PLATFORM_UNIX_ARM)
    set(CLR_CMAKE_PLATFORM_ARCH_ARM 1)
    set(CLR_CMAKE_HOST_ARCH "arm")
elseif(CLR_CMAKE_PLATFORM_UNIX_ARM64)
    set(CLR_CMAKE_PLATFORM_ARCH_ARM64 1)
    set(CLR_CMAKE_HOST_ARCH "arm64")
elseif(CLR_CMAKE_PLATFORM_UNIX_AMD64)
    set(CLR_CMAKE_PLATFORM_ARCH_AMD64 1)
    set(CLR_CMAKE_HOST_ARCH "x64")
elseif(CLR_CMAKE_PLATFORM_UNIX_X86)
    set(CLR_CMAKE_PLATFORM_ARCH_I386 1)
    set(CLR_CMAKE_HOST_ARCH "x86")
elseif(WIN32)
    # CLR_CMAKE_HOST_ARCH is passed in as param to cmake
    if (CLR_CMAKE_HOST_ARCH STREQUAL x64)
        set(CLR_CMAKE_PLATFORM_ARCH_AMD64 1)
    elseif(CLR_CMAKE_HOST_ARCH STREQUAL x86)
        set(CLR_CMAKE_PLATFORM_ARCH_I386 1)
    elseif(CLR_CMAKE_HOST_ARCH STREQUAL arm)
        set(CLR_CMAKE_PLATFORM_ARCH_ARM 1)
    elseif(CLR_CMAKE_HOST_ARCH STREQUAL arm64)
        set(CLR_CMAKE_PLATFORM_ARCH_ARM64 1)
    endif()
endif()

# Set TARGET architecture variables
# Target arch will be a cmake param (optional) for both windows as well as non-windows build
# if target arch is not specified then host & target are same
if(NOT DEFINED CLR_CMAKE_TARGET_ARCH OR CLR_CMAKE_TARGET_ARCH STREQUAL "" )
    set(CLR_CMAKE_TARGET_ARCH ${CLR_CMAKE_HOST_ARCH})
endif()

# Set target architecture variables
if (CLR_CMAKE_TARGET_ARCH STREQUAL x64)
    set(CLR_CMAKE_TARGET_ARCH_AMD64 1)
elseif(CLR_CMAKE_TARGET_ARCH STREQUAL x86)
    set(CLR_CMAKE_TARGET_ARCH_I386 1)
elseif(CLR_CMAKE_TARGET_ARCH STREQUAL arm64)
    set(CLR_CMAKE_TARGET_ARCH_ARM64 1)
elseif(CLR_CMAKE_TARGET_ARCH STREQUAL arm)
    set(CLR_CMAKE_TARGET_ARCH_ARM 1)
endif()

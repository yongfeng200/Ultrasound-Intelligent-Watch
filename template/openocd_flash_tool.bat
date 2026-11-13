@echo off
setlocal enabledelayedexpansion

:: ===================================================================
::               Loongarch-Link Chip Programmer
::             Copyright (C) 2024 by Wang Yufei
::                 All Rights Reserved.
::            Designed and Developed by Wang Yufei
::             Version: 1.0 | Last Update: 2024-11-29
:: ===================================================================
:: ===================================================================
:: Flash Tool Usage Guide
:: ===================================================================
:: 
:: Script Name: flash_tool.bat
:: Version: 1.0
:: Author: Wang Yufei
:: Description: This batch script provides tools to manage the flashing 
:: of firmware onto Loongarch-Link devices. It supports full chip 
:: erasure, address-based erasure, sector-based erasure, programming, 
:: and verifying of firmware images.
:: 
:: ===================================================================
:: USAGE
:: ===================================================================
::
:: This script can be invoked with one of the following commands:
::
:: 1. Erase the entire chip:
::    Command: erase_full
::    Usage: flash_tool.bat erase_full
::    Description: Erases the full flash memory of the chip.
::
:: 2. Erase flash memory from a specific address with a given size:
::    Command: erase_addr
::    Usage: flash_tool.bat erase_addr <address> <size>
::    Example: flash_tool.bat erase_addr 0x1f000000 0x80
::    Description: Erases the flash memory at the specified address and size.
::
:: 3. Erase a specified number of sectors starting from a given sector:
::    Command: erase_sectors
::    Usage: flash_tool.bat erase_sectors <start_sector> <sector_count>
::    Example: flash_tool.bat erase_sectors 0 10
::    Description: Erases a number of sectors starting from the given sector index.
::
:: 4. Program the firmware image to flash memory:
::    Command: program
::    Usage: flash_tool.bat program <firmware_file> <offset>
::    Example: flash_tool.bat program firmware.bin 0x1f000000
::    Description: Programs the specified firmware image at the given offset.
::
:: 5. Verify the firmware image against the flash memory:
::    Command: verify
::    Usage: flash_tool.bat verify <firmware_file> <offset>
::    Example: flash_tool.bat verify firmware.bin 0x1f000000
::    Description: Verifies the specified firmware image at the given offset.
::
:: 6. Program and verify firmware image:
::    Command: program_and_verify
::    Usage: flash_tool.bat program_and_verify <firmware_file> <offset>
::    Example: flash_tool.bat program_and_verify firmware.bin 0x1f000000
::    Description: Programs the firmware image at the given offset and immediately verifies it.
::
:: ===================================================================
:: PARAMETERS
:: ===================================================================
::
:: - <address>: The start address (in hexadecimal) where the flash memory 
::   should begin to be erased (for erase_addr).
:: 
:: - <size>: The size (in hexadecimal) of the flash memory section to erase 
::   (for erase_addr).
::
:: - <start_sector>: The index of the first sector to be erased (for erase_sectors).
::
:: - <sector_count>: The number of sectors to erase starting from the 
::   <start_sector> (for erase_sectors).
::
:: - <firmware_file>: The firmware image file (e.g., .bin or .hex format) to 
::   be flashed onto the device (for program, verify, and program_and_verify).
::
:: - <offset>: The offset (in hexadecimal) where the firmware image should 
::   be programmed or verified on the flash memory.
::
:: ===================================================================
:: EXAMPLES
:: ===================================================================
::
:: 1. Erase the full flash chip:
::    flash_tool.bat erase_full
::
:: 2. Erase from address 0x1f000000 with size 0x80:
::    flash_tool.bat erase_addr 0x1f000000 0x80
::
:: 3. Erase 10 sectors starting from sector 0:
::    flash_tool.bat erase_sectors 0 10
::
:: 4. Program firmware.bin at offset 0x1f000000:
::    flash_tool.bat program firmware.bin 0x1f000000
::
:: 5. Verify firmware.bin at offset 0x1f000000:
::    flash_tool.bat verify firmware.bin 0x1f000000
::
:: 6. Program and verify firmware.bin at offset 0x1f000000:
::    flash_tool.bat program_and_verify firmware.bin 0x1f000000
::
:: ===================================================================
:: COPYRIGHT & LICENSE
:: ===================================================================
::
:: Author: Wang Yufei
:: This script is free software; you can redistribute it and/or modify it 
:: under the terms of the MIT License.
:: 
:: This script is provided "as-is" without warranty of any kind. Use it at 
:: your own risk. The author is not responsible for any damage caused to 
:: hardware or software by using this script.
::
:: ===================================================================
:: END OF USAGE GUIDE
:: ===================================================================

:: ========== 配置与路径设置 ==========


:: 获取脚本所在目录
set "SCRIPT_DIR=%~dp0"
echo SCRIPT_DIR: %SCRIPT_DIR%

:: 去掉末尾的反斜杠
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

:: 获取父级目录（上级目录）
for %%I in ("%SCRIPT_DIR%\..") do set "PARENT_DIR=%%~fI"
echo PARENT_DIR: %PARENT_DIR%

:: 获取上上级目录（祖父级目录）
for %%I in ("%PARENT_DIR%\..") do set "GRANDPARENT_DIR=%%~fI"
echo GRANDPARENT_DIR: %GRANDPARENT_DIR%

:: 配置路径（调整 OpenOCD 路径）
set "OPENOCD_CMD=%GRANDPARENT_DIR%\openocd\bin\openocd.exe"
set "CONFIG_FILE=%GRANDPARENT_DIR%\openocd\bin\la-link.cfg"
set "TARGET_FILE=%GRANDPARENT_DIR%\openocd\bin\ls1d100.cfg"
set "FIRMWARE_DIR=%SCRIPT_DIR%\build\Debug"

:: 检查输入命令
if "%1"=="" (
    echo Usage: erase_full | erase_addr [address] [size] | erase_sectors [start_sector] [sector_count] | program [file] [offset] | verify [file] | program_and_verify [file] [offset]
    exit /b 1
)

:: 根据不同命令来处理
set "COMMAND=%1"
shift
echo COMMAND: %COMMAND%

:: 获取固件名称和偏移地址（仅在需要时）
if "%COMMAND%"=="program" (
    if "%1"=="" (
        echo Error: Missing firmware file name.
        exit /b 1
    )
    set "FIRMWARE_NAME=%1"
    shift
    echo FIRMWARE_NAME: %FIRMWARE_NAME%
)

if "%COMMAND%"=="program_and_verify" (
    if "%1"=="" (
        echo Error: Missing firmware file name.
        exit /b 1
    )
    set "FIRMWARE_NAME=%1"
    shift
    echo FIRMWARE_NAME: %FIRMWARE_NAME%
)

if "%COMMAND%"=="verify" (
    if "%1"=="" (
        echo Error: Missing firmware file name.
        exit /b 1
    )
    set "FIRMWARE_NAME=%1"
    shift
    echo FIRMWARE_NAME: %FIRMWARE_NAME%
)

:: 获取固件的完整路径
set "FIRMWARE_PATH=%FIRMWARE_DIR%\%FIRMWARE_NAME%"

:: 转换路径中的反斜杠为正斜杠（OpenOCD 兼容）
set "FIRMWARE_PATH=%FIRMWARE_PATH:\=/%"

:: 打印调试信息
 echo OPENOCD_CMD: %OPENOCD_CMD%
 echo CONFIG_FILE: %CONFIG_FILE%
 echo TARGET_FILE: %TARGET_FILE%
 echo FIRMWARE_DIR: %FIRMWARE_DIR%
 echo FIRMWARE_PATH: %FIRMWARE_PATH%

:: ========== 命令处理 ==========

:: 定义擦除整片函数
if "%COMMAND%"=="erase_full" (
    echo Erasing full chip...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 full;exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 full;exit;"
    exit /b 0
)

:: 定义按照地址擦除函数
if "%COMMAND%"=="erase_addr" (
    if "%1"=="" (
        echo Error: Missing address parameter.
        exit /b 1
    )
    if "%2"=="" (
        echo Error: Missing size parameter.
        exit /b 1
    )

    set "ADDR=%1"
    set "SIZE=%2"

    :: 打印调试信息，确认是否正确读取参数
    echo Address: !ADDR!
    echo Size: !SIZE!

    echo Erasing chip at address !ADDR! with size !SIZE%...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 addr !ADDR! !SIZE!;exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 addr !ADDR! !SIZE!;exit;"
    exit /b 0
)

:: 定义按照扇区擦除函数
if "%COMMAND%"=="erase_sectors" (
    if "%1"=="" (
        echo Error: Missing start sector parameter.
        exit /b 1
    )
    if "%2"=="" (
        echo Error: Missing sector count parameter.
        exit /b 1
    )
    set "START_SECTOR=%1"
    set "SECTOR_COUNT=%2"
    echo Erasing !SECTOR_COUNT! sectors starting from sector !START_SECTOR!...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 sectors !START_SECTOR! !SECTOR_COUNT!;exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;la132 mass_erase 0 sectors !START_SECTOR! !SECTOR_COUNT!;exit;"
    exit /b 0
)

:: 定义烧录并校验函数
if "%COMMAND%"=="program_and_verify" (
    if "%1"=="" (
        echo Error: Missing offset parameter.
        exit /b 1
    )
    set "OFFSET=%1"
    echo Programming and verifying file !FIRMWARE_NAME! at offset !OFFSET%...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "program !FIRMWARE_PATH! !OFFSET! verify reset exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "program !FIRMWARE_PATH! !OFFSET! verify reset exit;"
    exit /b 0
)

:: 定义烧录函数
if "%COMMAND%"=="program" (
    if "%1"=="" (
        echo Error: Missing offset parameter.
        exit /b 1
    )
    set "OFFSET=%1"
    echo Programming file !FIRMWARE_NAME! at offset !OFFSET%...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "program !FIRMWARE_PATH! !OFFSET! reset exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "program !FIRMWARE_PATH! !OFFSET! reset exit;"
    exit /b 0
)

:: 定义校验函数
if "%COMMAND%"=="verify" (
    if "%1"=="" (
        echo Error: Missing firmware file name.
        exit /b 1
    )
    set "FIRMWARE_NAME=%1"
    shift
    echo FIRMWARE_NAME: %FIRMWARE_NAME%

    if "%1"=="" (
        echo Error: Missing offset parameter.
        exit /b 1
    )
    set "OFFSET=%1"
    echo OFFSET: %OFFSET%

    :: 获取固件的完整路径
    set "FIRMWARE_PATH=%FIRMWARE_DIR%\%FIRMWARE_NAME%"
    :: 转换路径中的反斜杠为正斜杠（OpenOCD 兼容）
    set "FIRMWARE_PATH=%FIRMWARE_PATH:\=/%"

    echo Verifying firmware !FIRMWARE_NAME! at !FIRMWARE_PATH! with offset !OFFSET%...
    :: echo Running command: !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;flash verify_image !FIRMWARE_PATH! !OFFSET!;exit;"
    !OPENOCD_CMD! -f !CONFIG_FILE! -f !TARGET_FILE! -c "init;reset init;flash verify_image !FIRMWARE_PATH! !OFFSET!;exit;"
    exit /b 0
)


:: 如果没有匹配到任何命令
echo Invalid command. Usage: erase_full | erase_addr [address] [size] | erase_sectors [start_sector] [sector_count] | program [file] [offset] | verify [file] | program_and_verify [file] [offset]
exit /b 1

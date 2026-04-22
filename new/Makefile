# Makefile for BPLC_NW_STA ZB205 Project
# Generated from IAR project configuration
# Target: ARM Cortex-M4 (ZB204/ZB205)

#=============================================================================
# MSYS2 Configuration
#=============================================================================
# MSYS2 installation directory
#MSYS2_ROOT = D:/MSYS2
#MSYS2_BIN = $(MSYS2_ROOT)/mingw64/bin

# ARM toolchain prefix
PREFIX = arm-none-eabi-

# Toolchain path detection
# First check if MSYS2 toolchain exists, otherwise use system PATH
ifeq ($(OS),Windows_NT)
    # Windows: Check if MSYS2 toolchain exists
    ifneq ($(wildcard $(MSYS2_BIN)/$(PREFIX)gcc.exe),)
        TOOLCHAIN_PATH = $(MSYS2_BIN)/
    else
        TOOLCHAIN_PATH =
    endif
else
    # Linux/Unix: Use system PATH
    TOOLCHAIN_PATH =
endif

# Toolchain binaries
CC = $(TOOLCHAIN_PATH)$(PREFIX)gcc
AS = $(TOOLCHAIN_PATH)$(PREFIX)as
LD = $(TOOLCHAIN_PATH)$(PREFIX)ld
AR = $(TOOLCHAIN_PATH)$(PREFIX)ar
OBJCOPY = $(TOOLCHAIN_PATH)$(PREFIX)objcopy
SIZE = $(TOOLCHAIN_PATH)$(PREFIX)size

#=============================================================================
# Project Configuration
#=============================================================================
PROJECT_NAME = OS3
BUILD_DIR = gnu
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib

#=============================================================================
# Compiler and Linker Flags
#=============================================================================
# CPU configuration
CPU = -mcpu=cortex-m3
FPU = 
FLOAT-ABI = 

# MCU configuration
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Preprocessor definitions (from IAR project - FLASH configuration)
DEFINES = \
	-DNDEBUG \
	-DAES_WAFER_ID \
	-DZB205_CHIP \
	-DPROTOCOL_HRF

# Optimization level
OPT = -O1

# C standard
C_STANDARD = -std=gnu99

# Debug information
DEBUG = -g -gdwarf-2

# C compiler flags
CFLAGS = $(MCU) $(DEFINES) $(OPT) $(DEBUG) $(C_STANDARD)
CFLAGS += -Wall -fdata-sections -ffunction-sections
CFLAGS += -fno-strict-aliasing -fno-builtin
CFLAGS += -mthumb-interwork

# Assembler flags
ASFLAGS = $(MCU) $(DEBUG) -x assembler-with-cpp

#=============================================================================
# Include Paths
#=============================================================================
INCLUDES = \
	-I./app \
	-I./src/app \
	-I./maf/hrf/board \
	-I./maf/hrf/board/BSP \
	-I./maf/hrf/board/BSP/inc \
	-I./maf/hrf/GSMCU/CMSIS/Device/Include \
	-I./maf/hrf/GSMCU/HAL_Driver/Inc \
	-I./src \
	-I./src/app/inc \
	-I./src/aps/inc \
	-I./src/net/inc \
	-I./src/bplc \
	-I./src/phy \
	-I./src/common/inc \
	-I./src/common/finsh/inc \
	-I./src/cco \
	-I./version \
	-I./config \
	-I./lzma2106 \
	-I./ucosIII/uC-CPU \
	-I./ucosIII/uC-CPU/ARM-Cortex-M4 \
	-I./ucosIII/uC-LIB \
	-I./ucosIII/uC-LIB/Ports/ARM-Cortex-M4 \
	-I./ucosIII/uCOS-III/Source \
	-I./ucosIII/uCOS-III/Ports/ARM-Cortex-M4/Generic \
	-I./maf/hrf/GSMCU/CMSIS/Include/GCC/Core/Include \
	-I./maf/hrf/GSMCU/CMSIS/Include/GCC/Driver/Include \
	-I./maf/hrf/GSMCU/CMSIS/Include \
	-I./ucosIII/uC-CPU/ARM-Cortex-M4/IAR \
	-I./ucosIII/uCOS-III/Ports/ARM-Cortex-M4/Generic/IAR \
	-I./src/encry \
	-I./maf/hrf/GSMCU/CMSIS/GCC/Core/Include \
	-I./maf/hrf/GSMCU/CMSIS/GCC/Driver/Include \
	-I./src/net \
	-I./src \
    -I./src/app \
    -I./src/aps \
    -I./src/net \
    -I./src/bplc \
    -I./src/phy \
    -I./src/common \
    -I./src \
    -I./src/cco \
    -I./version \
    -I./config \
    -I./src/encry \

	

#=============================================================================
# Linker Configuration
#=============================================================================
# Linker script
LDSCRIPT = icf/sram.ld

# Linker flags
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(DEBUG)
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

# Libraries
LIBS = -lc -lm -lnosys

#=============================================================================
# Source Files
#=============================================================================

# APP source files
C_SOURCES_APP = \
	app/app.c \
	app/os_app_hooks.c

# BOARD HAL driver source files
C_SOURCES_BOARD = \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_cmp_lvd.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_cortex.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_crc.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_dma.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_flash.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_iwdg.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_gpio.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_pit32.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_port.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_pwm.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_spi.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_m3_usart.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_scu.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcu_trng.c \
	maf/hrf/GSMCU/HAL_Driver/Src/gsmcuxx_hal.c \
	maf/hrf/GSMCU/HAL_Driver/Src/system_GSMCU_M3.c

# BSP source files
C_SOURCES_BSP = \
	maf/hrf/board/BSP/bps_debug.c \
	maf/hrf/board/BSP/bps_flash.c \
	maf/hrf/board/BSP/bps_led.c \
	maf/hrf/board/BSP/bps_port.c \
	maf/hrf/board/BSP/bps_timer.c \
	maf/hrf/board/BSP/bps_uart.c \
	maf/hrf/board/BSP/bps_zerocross.c \
	maf/hrf/board/BSP/bsp.c \
	maf/hrf/board/BSP/bsp_int.c \
	maf/hrf/board/BSP/bsp_wdg.c \
	maf/hrf/board/BSP/cpu_bsp.c \
	maf/hrf/board/BSP/IAR/cstartup.c

# uC/OS-III source files
C_SOURCES_UCOSIII = \
	ucosIII/uC-CPU/cpu_core.c \
	ucosIII/uC-LIB/lib_ascii.c \
	ucosIII/uC-LIB/lib_math.c \
	ucosIII/uC-LIB/lib_mem.c \
	ucosIII/uC-LIB/lib_str.c \
	ucosIII/uCOS-III/Source/os_cfg_app.c \
	ucosIII/uCOS-III/Source/os_core.c \
	ucosIII/uCOS-III/Source/os_dbg.c \
	ucosIII/uCOS-III/Source/os_flag.c \
	ucosIII/uCOS-III/Source/os_int.c \
	ucosIII/uCOS-III/Source/os_mem.c \
	ucosIII/uCOS-III/Source/os_msg.c \
	ucosIII/uCOS-III/Source/os_mutex.c \
	ucosIII/uCOS-III/Source/os_pend_multi.c \
	ucosIII/uCOS-III/Source/os_prio.c \
	ucosIII/uCOS-III/Source/os_q.c \
	ucosIII/uCOS-III/Source/os_sem.c \
	ucosIII/uCOS-III/Source/os_stat.c \
	ucosIII/uCOS-III/Source/os_task.c \
	ucosIII/uCOS-III/Source/os_tick.c \
	ucosIII/uCOS-III/Source/os_time.c \
	ucosIII/uCOS-III/Source/os_tmr.c \
	ucosIII/uCOS-III/Source/os_var.c \
	ucosIII/uCOS-III/Ports/ARM-Cortex-M4/Generic/IAR/os_cpu_c.c

# APP layer source files
C_SOURCES_SRC_APP = \
	src/app/init.c

# APS layer source files
C_SOURCES_SRC_APS = \
	src/aps/local_protocol.c

# CCO application source files
C_SOURCES_CCO = \
	src/cco/alert.c \
	src/cco/CCO_app.c \
	src/cco/Manager_link.c \
	src/cco/module_def.c \
	src/cco/PLC_AutoRelay.c \
	src/cco/PLM_2005.c \
	src/cco/PLM_AFN.c \
	src/cco/PLM_AFN_GD.c \
	src/cco/protocol_645.c \
	src/cco/protocol_69845.c \
	src/cco/public_func.c \
	src/cco/queue.c \
	src/cco/RTC.c \
	src/cco/SHA1.c \
	src/cco/soft_check.c \
	src/cco/storage.c \
	src/cco/Uart_Link.c

# Common utility source files
C_SOURCES_COMMON = \
	src/common/algorithm.c \
	src/common/common_includes.c \
	src/common/fsm.c \
	src/common/list.c \
	src/common/map.c \
	src/common/rbtree.c \
	src/common/timer.c \
	src/common/tmr_call.c

# Encryption source files
C_SOURCES_ENCRY = \
	src/encry/aes.c \
	src/encry/crypto.c \
	src/encry/sha.c \
	src/encry/sm4.c

# MAC library source files
C_SOURCES_MAC = \
                src\net\hplc_beacon.c \
                src\net\hplc_channel_status.c \
                src\net\hplc_data.c \
                src\net\hplc_frame.c \
                src\net\hplc_mac.c \
                src\net\hplc_mac_def.c \
                src\net\hplc_mpdu.c \
                src\net\hplc_offline.c \
                src\net\hplc_protocol_init.c \
                src\net\hplc_receive.c \
                src\net\hplc_route_period.c \
                src\net\hplc_send.c \
                src\net\hplc_timer.c \



# Network layer source files (excluding MAC library files)
C_SOURCES_NET = \
	src/net/hplc_app.c \
	src/net/hplc_app_def.c \
	src/net/hplc_task.c 

# PHY library source files (these are in the prebuilt libphy.a)
C_SOURCES_PHY_LIB = \
	src/phy/hrf_port_zb205.c \
	src/phy/hrf_table_zb205.c \
	src/phy/phy_driver.c \
	src/phy/phy_port_zb205.c \
	src/phy/phy_table_zb205.c \
	src/phy/phy_zerocross.c

# Physical layer source files (application-level PHY code, not in library)
C_SOURCES_PHY = \
	src/phy/hard_aes.c \
	src/phy/hrf_phy.c \
	src/phy/hplc_phy.c \
	src/phy/hrf_driver.c \
	src/phy/phy_timer.c \
	src/phy/phy_task.c

# SystemView source files (not used in uC/OS-III configuration)
# C_SOURCES_SYSVIEW = \
# 	SystemView/Src/Sample/FreeRTOSV10/Config/Cortex-M/SEGGER_SYSVIEW_Config_FreeRTOS.c \
# 	SystemView/Src/Sample/FreeRTOSV10/SEGGER_SYSVIEW_FreeRTOS.c \
# 	SystemView/Src/SEGGER/SEGGER_RTT.c \
# 	SystemView/Src/SEGGER/SEGGER_RTT_printf.c \
# 	SystemView/Src/SEGGER/SEGGER_SYSVIEW.c

# LZMA compression source files
#C_SOURCES_ZIP = \
#	lzma2106/Alloc.c \
#	lzma2106/LzmaDec.c \
#	lzma2106/LzmaLib.c

# Assembly source files
ASM_SOURCES = \
	ucosIII/uCOS-III/Ports/ARM-Cortex-M4/Generic/GCC/os_cpu_a.s \
	ucosIII/uC-CPU/ARM-Cortex-M4/GCC/cpu_a.s \
	ucosIII/uC-LIB/Ports/ARM-Cortex-M4/GCC/lib_mem_a.s

# All C source files combined (for full build with library compilation)
C_SOURCES_WITH_LIBS = \
	$(C_SOURCES_APP) \
	$(C_SOURCES_BOARD) \
	$(C_SOURCES_BSP) \
	$(C_SOURCES_UCOSIII) \
	$(C_SOURCES_SRC_APP) \
	$(C_SOURCES_SRC_APS) \
	$(C_SOURCES_CCO) \
	$(C_SOURCES_COMMON) \
	$(C_SOURCES_ENCRY) \
	$(C_SOURCES_MAC) \
	$(C_SOURCES_NET) \
	$(C_SOURCES_PHY) \
	$(C_SOURCES_PHY_LIB) \
	#$(C_SOURCES_ZIP)

# Application source files (excluding library sources - for prebuilt libs build)
C_SOURCES = \
	$(C_SOURCES_APP) \
	$(C_SOURCES_BOARD) \
	$(C_SOURCES_BSP) \
	$(C_SOURCES_UCOSIII) \
	$(C_SOURCES_SRC_APP) \
	$(C_SOURCES_SRC_APS) \
	$(C_SOURCES_CCO) \
	$(C_SOURCES_COMMON) \
	$(C_SOURCES_ENCRY) \
	$(C_SOURCES_NET) \
	$(C_SOURCES_PHY) \
	#$(C_SOURCES_ZIP)

#=============================================================================
# Object Files
#=============================================================================
# MAC library objects
MAC_OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES_MAC:.c=.o)))

# PHY library objects
PHY_OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES_PHY_LIB:.c=.o)))

# Main application objects (excluding library sources)
OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES:.c=.o)))

# Set vpath for all C source files (combined to avoid overwriting)
vpath %.c $(sort $(dir $(C_SOURCES_WITH_LIBS)))

# Handle .s files (lowercase)
ASM_SOURCES_LOWER = $(filter %.s,$(ASM_SOURCES))
OBJECTS += $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES_LOWER:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES_LOWER)))

# Handle .S files (uppercase)
ASM_SOURCES_UPPER = $(filter %.S,$(ASM_SOURCES))
OBJECTS += $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES_UPPER:.S=.o)))
vpath %.S $(sort $(dir $(ASM_SOURCES_UPPER)))

#=============================================================================
# Build Targets
#=============================================================================
.PHONY: all clean directories lib libs with-prebuilt-libs

# Full build: compile everything including library sources, then create libraries
all: directories $(BIN_DIR)/$(PROJECT_NAME)_full.elf $(BIN_DIR)/$(PROJECT_NAME)_full.hex $(BIN_DIR)/$(PROJECT_NAME)_full.bin lib
	@echo ""
	@echo "=========================================="
	@echo "Full build completed!"
	@echo "=========================================="
	@echo "Libraries created: $(LIB_DIR)/libmac.a, $(LIB_DIR)/libphy.a"
	@echo ""

# Build with prebuilt libraries (for outsourcing - no library source compilation)
with-prebuilt-libs: directories $(BIN_DIR)/$(PROJECT_NAME).elf $(BIN_DIR)/$(PROJECT_NAME).hex $(BIN_DIR)/$(PROJECT_NAME).bin
	@echo ""
	@echo "=========================================="
	@echo "Build completed using prebuilt libraries!"
	@echo "=========================================="
	@echo "This build uses precompiled MAC and PHY libraries."
	@echo "Library source code is not required for this build."
	@echo ""

# Create output directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(LIB_DIR)

# Build libraries from compiled objects (standalone target)
lib: directories $(LIB_DIR)/libmac.a $(LIB_DIR)/libphy.a
	@echo ""
	@echo "=========================================="
	@echo "Libraries built successfully!"
	@echo "=========================================="
	@echo ""

# Alias for lib target
libs: lib

# Build MAC library
$(LIB_DIR)/libmac.a: $(MAC_OBJECTS)
	@echo "Creating MAC library: $@"
	$(AR) rcs $@ $^

# Build PHY library
$(LIB_DIR)/libphy.a: $(PHY_OBJECTS)
	@echo "Creating PHY library: $@"
	$(AR) rcs $@ $^

# Link full build ELF (all sources compiled directly, no libraries used)
OBJECTS_FULL = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES_WITH_LIBS:.c=.o)))
$(BIN_DIR)/$(PROJECT_NAME)_full.elf: $(OBJECTS_FULL) $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES_LOWER:.s=.o))) $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES_UPPER:.S=.o)))
	@echo "Linking (full build): $@"
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

# Link to generate ELF file (using prebuilt libraries)
# Note: This target assumes libmac.a and libphy.a already exist in $(LIB_DIR)
$(BIN_DIR)/$(PROJECT_NAME).elf: $(OBJECTS)
	@echo ""
	@echo "NOTE: This build mode uses precompiled MAC and PHY libraries."
	@echo "      Library source code is NOT required for this build."
	@echo ""
	@if [ ! -f "$(LIB_DIR)/libmac.a" ]; then \
		echo ""; \
		echo "ERROR: $(LIB_DIR)/libmac.a not found!"; \
		echo "Please run 'make all' first to build the libraries, or"; \
		echo "ensure prebuilt libraries are placed in $(LIB_DIR)/"; \
		echo ""; \
		echo "Make sure the following library files exist:"; \
		echo "  - $(LIB_DIR)/libmac.a"; \
		echo "  - $(LIB_DIR)/libphy.a"; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f "$(LIB_DIR)/libphy.a" ]; then \
		echo ""; \
		echo "ERROR: $(LIB_DIR)/libphy.a not found!"; \
		echo "Please run 'make all' first to build the libraries, or"; \
		echo "ensure prebuilt libraries are placed in $(LIB_DIR)/"; \
		echo ""; \
		echo "Make sure the following library files exist:"; \
		echo "  - $(LIB_DIR)/libmac.a"; \
		echo "  - $(LIB_DIR)/libphy.a"; \
		echo ""; \
		exit 1; \
	fi
	@echo "Linking (with prebuilt libs): $@"
	$(CC) $(OBJECTS) -L$(LIB_DIR) -lmac -lphy $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

# Generate HEX file
$(BIN_DIR)/%.hex: $(BIN_DIR)/%.elf
	@echo "Creating HEX: $@"
	$(OBJCOPY) -O ihex $< $@

# Generate BIN file
$(BIN_DIR)/%.bin: $(BIN_DIR)/%.elf
	@echo "Creating BIN: $@"
	$(OBJCOPY) -O binary -S $< $@

# Compile C source files
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling: $<"
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Assemble files (use GCC for assembler-with-cpp support)
$(OBJ_DIR)/%.o: %.s
	@echo "Assembling: $<"
	$(CC) -c $(ASFLAGS) $(INCLUDES) $< -o $@

$(OBJ_DIR)/%.o: %.S
	@echo "Assembling: $<"
	$(CC) -c $(ASFLAGS) $(INCLUDES) $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)

# Display help information
help:
	@echo "Available targets:"
	@echo "  all                  - Build all targets including libraries (default)"
	@echo "  with-prebuilt-libs   - Build using prebuilt MAC/PHY libraries (for outsourcing)"
	@echo "  lib                  - Build only MAC and PHY libraries"
	@echo "  libs                 - Alias for 'lib' target"
	@echo "  clean                - Remove all build files"
	@echo "  help                 - Display this help information"
	@echo ""
	@echo "Build output: $(BUILD_DIR)/"
	@echo "  - $(LIB_DIR)/  : Library files (libmac.a, libphy.a)"
	@echo "  - $(OBJ_DIR)/  : Object files"
	@echo "  - $(BIN_DIR)/  : Final binary files (ELF, HEX, BIN)"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                      - Full build with library compilation"
	@echo "  make with-prebuilt-libs   - Build using existing libraries (no lib source needed)"
	@echo "  make lib                  - Build only the libraries"
	@echo "  make clean                - Clean all build artifacts"

#=============================================================================
# Dependency Files
#=============================================================================
-include $(wildcard $(OBJ_DIR)/*.d)
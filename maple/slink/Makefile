EXE_NAME = build/slink.elf
OBJS = build/slink.o build/TimerControl.o
CXX = arm-none-eabi-g++
CC = arm-none-eabi-gcc

MAPLE_PATH=./maple

DEFINES = -DVECT_TAB_FLASH \
	-DBOARD_maple \
	-DMCU_STM32F103RB \
	-DERROR_LED_PORT=GPIOA \
	-DERROR_LED_PIN=5 \
	-DSTM32_MEDIUM_DENSITY

INCLUDES=-I$(MAPLE_PATH)/libmaple \
	-I$(MAPLE_PATH)/libmaple/usb \
	-I$(MAPLE_PATH)/libmaple/usb/usb_lib \
	-I$(MAPLE_PATH)/wirish \
	-I$(MAPLE_PATH)/libraries/Servo \
	-I$(MAPLE_PATH)/libraries/LiquidCrystal \
	-I$(MAPLE_PATH)/libraries/Wire \
	-I$(MAPLE_PATH)/libraries/FreeRTOS \
	-I$(MAPLE_PATH)/wirish \
	-I$(MAPLE_PATH)/wirish/comm \
	-I$(MAPLE_PATH)/wirish/boards \

GLOBAL_CFLAGS := -Os -g3 -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -nostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections $(DEFINES)
GLOBAL_CXXFLAGS := -fno-rtti -fno-exceptions -Wall $(DEFINES)
GLOBAL_ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp $(DEFINES)

LDDIR    := $(MAPLE_PATH)/ld
LDFLAGS  = -T$(LDDIR)/$(LDSCRIPT) -L$(LDDIR)    \
            -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall

$(EXE_NAME): $(OBJS)
	$(CXX) $< -c $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) $(INCLUDES)
	arm-none-eabi-objcopy -v -Obinary build/slink.elf build/slink.bin 1>/dev/null
	arm-none-eabi-objdump -d build/slink.elf > build/slink.disas

build: 
	@ mkdir -p build

build/%.o: %.cpp
	$(CXX) $< -c $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) $(INCLUDES)


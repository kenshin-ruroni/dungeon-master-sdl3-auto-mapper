################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/AsciiDump.cpp \
../src/Attack.cpp \
../src/Bitmaps.cpp \
../src/CSBCode.cpp \
../src/CSBUI.cpp \
../src/CSBlinux.cpp \
../src/Chaos.cpp \
../src/Character.cpp \
../src/Code11f52.cpp \
../src/Code13ea4.cpp \
../src/Code17818.cpp \
../src/Code1f9e6.cpp \
../src/Code222ea.cpp \
../src/Code390e.cpp \
../src/Code51a4.cpp \
../src/Codea59a.cpp \
../src/DSA.cpp \
../src/Graphics.cpp \
../src/Hint.cpp \
../src/LinCSBUI.cpp \
../src/LinScreen.cpp \
../src/Magic.cpp \
../src/Menu.cpp \
../src/Monster.cpp \
../src/Mouse.cpp \
../src/MoveObject.cpp \
../src/NewBugs.cpp \
../src/RC4.cpp \
../src/Recording.cpp \
../src/SaveGame.cpp \
../src/SmartDiscard.cpp \
../src/Sound.cpp \
../src/Statistics.cpp \
../src/Stdafx.cpp \
../src/Timer.cpp \
../src/Translation.cpp \
../src/VBL.cpp \
../src/Viewport.cpp \
../src/data.cpp \
../src/md5C.cpp \
../src/system.cpp \
../src/utility.cpp 

CPP_DEPS += \
./src/AsciiDump.d \
./src/Attack.d \
./src/Bitmaps.d \
./src/CSBCode.d \
./src/CSBUI.d \
./src/CSBlinux.d \
./src/Chaos.d \
./src/Character.d \
./src/Code11f52.d \
./src/Code13ea4.d \
./src/Code17818.d \
./src/Code1f9e6.d \
./src/Code222ea.d \
./src/Code390e.d \
./src/Code51a4.d \
./src/Codea59a.d \
./src/DSA.d \
./src/Graphics.d \
./src/Hint.d \
./src/LinCSBUI.d \
./src/LinScreen.d \
./src/Magic.d \
./src/Menu.d \
./src/Monster.d \
./src/Mouse.d \
./src/MoveObject.d \
./src/NewBugs.d \
./src/RC4.d \
./src/Recording.d \
./src/SaveGame.d \
./src/SmartDiscard.d \
./src/Sound.d \
./src/Statistics.d \
./src/Stdafx.d \
./src/Timer.d \
./src/Translation.d \
./src/VBL.d \
./src/Viewport.d \
./src/data.d \
./src/md5C.d \
./src/system.d \
./src/utility.d 

OBJS += \
./src/AsciiDump.o \
./src/Attack.o \
./src/Bitmaps.o \
./src/CSBCode.o \
./src/CSBUI.o \
./src/CSBlinux.o \
./src/Chaos.o \
./src/Character.o \
./src/Code11f52.o \
./src/Code13ea4.o \
./src/Code17818.o \
./src/Code1f9e6.o \
./src/Code222ea.o \
./src/Code390e.o \
./src/Code51a4.o \
./src/Codea59a.o \
./src/DSA.o \
./src/Graphics.o \
./src/Hint.o \
./src/LinCSBUI.o \
./src/LinScreen.o \
./src/Magic.o \
./src/Menu.o \
./src/Monster.o \
./src/Mouse.o \
./src/MoveObject.o \
./src/NewBugs.o \
./src/RC4.o \
./src/Recording.o \
./src/SaveGame.o \
./src/SmartDiscard.o \
./src/Sound.o \
./src/Statistics.o \
./src/Stdafx.o \
./src/Timer.o \
./src/Translation.o \
./src/VBL.o \
./src/Viewport.o \
./src/data.o \
./src/md5C.o \
./src/system.o \
./src/utility.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_LINUX -DRGB655 -O0 -g3 -Wall -c -fmessage-length=0 -fpermissive -Wno-switch -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/AsciiDump.d ./src/AsciiDump.o ./src/Attack.d ./src/Attack.o ./src/Bitmaps.d ./src/Bitmaps.o ./src/CSBCode.d ./src/CSBCode.o ./src/CSBUI.d ./src/CSBUI.o ./src/CSBlinux.d ./src/CSBlinux.o ./src/Chaos.d ./src/Chaos.o ./src/Character.d ./src/Character.o ./src/Code11f52.d ./src/Code11f52.o ./src/Code13ea4.d ./src/Code13ea4.o ./src/Code17818.d ./src/Code17818.o ./src/Code1f9e6.d ./src/Code1f9e6.o ./src/Code222ea.d ./src/Code222ea.o ./src/Code390e.d ./src/Code390e.o ./src/Code51a4.d ./src/Code51a4.o ./src/Codea59a.d ./src/Codea59a.o ./src/DSA.d ./src/DSA.o ./src/Graphics.d ./src/Graphics.o ./src/Hint.d ./src/Hint.o ./src/LinCSBUI.d ./src/LinCSBUI.o ./src/LinScreen.d ./src/LinScreen.o ./src/Magic.d ./src/Magic.o ./src/Menu.d ./src/Menu.o ./src/Monster.d ./src/Monster.o ./src/Mouse.d ./src/Mouse.o ./src/MoveObject.d ./src/MoveObject.o ./src/NewBugs.d ./src/NewBugs.o ./src/RC4.d ./src/RC4.o ./src/Recording.d ./src/Recording.o ./src/SaveGame.d ./src/SaveGame.o ./src/SmartDiscard.d ./src/SmartDiscard.o ./src/Sound.d ./src/Sound.o ./src/Statistics.d ./src/Statistics.o ./src/Stdafx.d ./src/Stdafx.o ./src/Timer.d ./src/Timer.o ./src/Translation.d ./src/Translation.o ./src/VBL.d ./src/VBL.o ./src/Viewport.d ./src/Viewport.o ./src/data.d ./src/data.o ./src/md5C.d ./src/md5C.o ./src/system.d ./src/system.o ./src/utility.d ./src/utility.o

.PHONY: clean-src


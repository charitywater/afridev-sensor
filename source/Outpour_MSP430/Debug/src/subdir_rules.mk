################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
src/CTS_HAL.obj: ../src/CTS_HAL.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/CTS_HAL.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/CTS_Layer.obj: ../src/CTS_Layer.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/CTS_Layer.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/RTC_Calendar.obj: ../src/RTC_Calendar.asm $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/RTC_Calendar.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/flash.obj: ../src/flash.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/flash.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/hal.obj: ../src/hal.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/hal.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/main.obj: ../src/main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/main.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/modemCmd.obj: ../src/modemCmd.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/modemCmd.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/modemLink.obj: ../src/modemLink.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/modemLink.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/modemMgr.obj: ../src/modemMgr.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/modemMgr.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/msgData.obj: ../src/msgData.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/msgData.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/msgDataSm.obj: ../src/msgDataSm.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/msgDataSm.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/msgDebug.obj: ../src/msgDebug.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/msgDebug.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/msgOta.obj: ../src/msgOta.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/msgOta.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/storage.obj: ../src/storage.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/storage.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/structure.obj: ../src/structure.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/structure.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/sysExec.obj: ../src/sysExec.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/sysExec.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/time.obj: ../src/time.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/time.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/utils.obj: ../src/utils.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/utils.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/waterSense.obj: ../src/waterSense.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmsp --abi=eabi -O4 --opt_for_speed=0 --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --gcc --define=__MSP430G2553__ --define=FOR_USE_WITH_BOOTLOADER --display_error_number --diag_wrap=off --printf_support=minimal --preproc_with_compile --preproc_dependency="src/waterSense.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



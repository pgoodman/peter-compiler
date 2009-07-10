################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/std/assert.c \
../src/std/input.c \
../src/std/memory.c \
../src/std/string.c 

OBJS += \
./src/std/assert.o \
./src/std/input.o \
./src/std/memory.o \
./src/std/string.o 

C_DEPS += \
./src/std/assert.d \
./src/std/input.d \
./src/std/memory.d \
./src/std/string.d 


# Each subdirectory must supply rules for building sources it contributes
src/std/%.o: ../src/std/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/petergoodman/Documents/workspace/P_Compiler/src/headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



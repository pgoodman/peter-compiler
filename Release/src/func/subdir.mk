################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/func/delegate.c \
../src/func/function.c \
../src/func/predicate.c 

OBJS += \
./src/func/delegate.o \
./src/func/function.o \
./src/func/predicate.o 

C_DEPS += \
./src/func/delegate.d \
./src/func/function.d \
./src/func/predicate.d 


# Each subdirectory must supply rules for building sources it contributes
src/func/%.o: ../src/func/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/petergoodman/Documents/workspace/P_Compiler/src/headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/vendor/murmur-hash.c 

OBJS += \
./src/vendor/murmur-hash.o 

C_DEPS += \
./src/vendor/murmur-hash.d 


# Each subdirectory must supply rules for building sources it contributes
src/vendor/%.o: ../src/vendor/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/petergoodman/Documents/workspace/P_Compiler/src/headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



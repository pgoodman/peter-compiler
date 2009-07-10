################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/p/grammar.c \
../src/p/parse-tree.c \
../src/p/parser.c \
../src/p/regexp.c \
../src/p/scanner.c 

OBJS += \
./src/p/grammar.o \
./src/p/parse-tree.o \
./src/p/parser.o \
./src/p/regexp.o \
./src/p/scanner.o 

C_DEPS += \
./src/p/grammar.d \
./src/p/parse-tree.d \
./src/p/parser.d \
./src/p/regexp.d \
./src/p/scanner.d 


# Each subdirectory must supply rules for building sources it contributes
src/p/%.o: ../src/p/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/petergoodman/Documents/workspace/P_Compiler/src/headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



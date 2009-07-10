################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/adt/dict.c \
../src/adt/generator.c \
../src/adt/list.c \
../src/adt/nfa.c \
../src/adt/queue.c \
../src/adt/set.c \
../src/adt/stack.c \
../src/adt/tree.c \
../src/adt/vector.c 

OBJS += \
./src/adt/dict.o \
./src/adt/generator.o \
./src/adt/list.o \
./src/adt/nfa.o \
./src/adt/queue.o \
./src/adt/set.o \
./src/adt/stack.o \
./src/adt/tree.o \
./src/adt/vector.o 

C_DEPS += \
./src/adt/dict.d \
./src/adt/generator.d \
./src/adt/list.d \
./src/adt/nfa.d \
./src/adt/queue.d \
./src/adt/set.d \
./src/adt/stack.d \
./src/adt/tree.d \
./src/adt/vector.d 


# Each subdirectory must supply rules for building sources it contributes
src/adt/%.o: ../src/adt/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/petergoodman/Documents/workspace/P_Compiler/src/headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



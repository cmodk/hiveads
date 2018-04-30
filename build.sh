#!/bin/bash
gcc -g -Isrc/devicehive-logger/src -o hiveads main.c \
	src/devicehive-logger/src/data_logger_helper.c \
	src/devicehive-logger/src/devicehive.c \
	src/devicehive-logger/src/timestamp.c -lcurl

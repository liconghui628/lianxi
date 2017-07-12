#!/bin/bash

CC=gcc

.PHONY : all

all : logtest logcat

logtest:logtest.c bl_log.c bl_shm.c bl_sem.c bl_log.h bl_shm.h bl_sem.h
	$(CC) logtest.c bl_log.c bl_shm.c bl_sem.c bl_log.h bl_shm.h bl_sem.h -o logtest

logcat:logcat.c bl_log.c bl_shm.c bl_sem.c bl_log.h bl_shm.h bl_sem.h
	$(CC) logcat.c bl_log.c bl_shm.c bl_sem.c bl_log.h bl_shm.h bl_sem.h -o logcat



.PHONY : clean
clean : 
	-rm -f logtest logcat 

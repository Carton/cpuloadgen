#/*
# *
# * @Component			CPULOADGEN
# * @Filename			Makefile
# * @Description		Makefile for cpuloadgen
# * @Author			Patrick Titiano (p-titiano@ti.com)
# * @Date			2010
# * @Copyright			Texas Instruments Incorporated
# *
# *
# * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
# *
# *
# *  Redistribution and use in source and binary forms, with or without
# *  modification, are permitted provided that the following conditions
# *  are met:
# *
# *    Redistributions of source code must retain the above copyright
# *    notice, this list of conditions and the following disclaimer.
# *
# *    Redistributions in binary form must reproduce the above copyright
# *    notice, this list of conditions and the following disclaimer in the
# *    documentation and/or other materials provided with the
# *    distribution.
# *
# *    Neither the name of Texas Instruments Incorporated nor the names of
# *    its contributors may be used to endorse or promote products derived
# *    from this software without specific prior written permission.
# *
# *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# *
# */


CC = $(CROSS_COMPILE)gcc
MYCFLAGS += -Wall -static
DESTDIR = ./out

objects = cpuloadgen.o timers_b.o dhry_21b.o

cpuloadgen: $(objects) builddate.o dhry.h
	$(CC) $(MYCFLAGS) -o cpuloadgen $(objects) builddate.o
	rm builddate.c

builddate.c: $(objects)
	echo 'char *builddate="'`date`'";' > builddate.c

install: cpuloadgen
	install -d $(DESTDIR)
	install cpuloadgen $(DESTDIR)

clean:
	rm -f cpuloadgen $(objects) builddate.o builddate.c
/*
 *
 * @Component			CPULOADGEN
 * @Filename			cpuloadgen.c
 * @Description			Programmable CPU Load Generator
 * @Author			Patrick Titiano (p-titiano@ti.com)
 * @Date			2010
 * @Copyright			Texas Instruments Incorporated
 *
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#define __USE_GNU
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define CPULOADGEN_REVISION ((const char *) "0.94")

/* #define CPU_AFFINITY */
/* #define DEBUG */
#ifdef DEBUG
#define dprintf(format, ...)	 printf(format, ## __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif


#ifndef ROPT
#define REG
#else
#define REG register
#endif

/* Global Variables: */

extern double dtime();

int cpu_count = -1;
int *cpuloads = NULL;
long int duration = -1;
pthread_t *threads = NULL;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void workload(unsigned int iterations);
void loadgen(unsigned int cpu, unsigned int load, unsigned int duration);

/* ------------------------------------------------------------------------*//**
 * @FUNCTION		usage
 * @BRIEF		Display list of supported commands.
 * @DESCRIPTION		Display list of supported commands.
 *//*------------------------------------------------------------------------ */
static void usage(void)
{
	printf("Usage:\n");
	printf("\tcpuloadgen [<cpu[n]=load>] [<duration=time>]\n\n");
	printf("Generate adjustable processing load on selected CPU core(s) for a given duration.\n");
	printf("Load is a percentage which may be any integer value between 1 and 100.\n");
	printf("Duration time unit is seconds.\n");
	printf("Arguments may be provided in any order.\n");
	printf("If duration is omitted, generate load(s) until CTRL+C is pressed.\n");
	printf("If no argument is given, generate 100%% load on all online CPU cores indefinitely.\n\n");
	printf("e.g.:\n");
	printf(" - Generate 100%% load on all online CPU cores until CTRL+C is pressed:\n");
	printf("	# cpuloadgen\n");
	printf(" - Generate 100%% load on all online CPU cores during 10 seconds:\n");
	printf("	# cpuloadgen duration=10\n");
	printf(" - Generate 50%% load on CPU1 and 100%% load on CPU3 during 10 seconds:\n");
	printf("	# cpuloadgen cpu3=100 cpu1=50 duration=5\n\n");
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		free_buffers
 * @BRIEF		free allocated buffers.
 * @DESCRIPTION		free allocated buffers.
 *//*------------------------------------------------------------------------ */
static void free_buffers(void)
{
	if (threads != NULL)
		free(threads);
	if (cpuloads != NULL)
		free(cpuloads);
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		einval
 * @BRIEF		Display standard message in case of invalid argument.
 * @RETURNS		-EINVAL
 * @param[in]		arg: invalid argument
 * @DESCRIPTION		Display standard message in case of invalid argument.
 *			Take care of freeing allocated buffers
 *//*------------------------------------------------------------------------ */
static int einval(const char *arg)
{
	fprintf(stderr, "cpuloadgen: invalid argument!!! (%s)\n\n", arg);
	usage();
	free_buffers();
	return -EINVAL;
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		sigterm_handler
 * @BRIEF		parent SIGTERM callback function.
 *			Send SIGTERM signal to child process.
 * @DESCRIPTION		parent SIGTERM callback function.
 *			Send SIGTERM signal to child process.
 *//*------------------------------------------------------------------------ */
void sigterm_handler(void)
{
	printf("Halting load generation...\n");
	fflush(stdout);

	free_buffers();

	printf("done.\n\n");
	fflush(stdout);
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		thread_loadgen
 * @BRIEF		pthread wrapper around loadgen() function.
 * @param[in]		ptr: pointer to the cpu core id
 * @DESCRIPTION		pthread wrapper around loadgen() function.
 *//*------------------------------------------------------------------------ */
void *thread_loadgen(void *ptr)
{
	unsigned int cpu;

	cpu = *((unsigned int *) ptr);
	pthread_mutex_unlock(&mutex1);
	if (cpu < (unsigned int)cpu_count) {
		loadgen(cpu, cpuloads[cpu], duration);
	} else {
		fprintf(stderr, "%s: invalid cpu argument!!! (%d)\n",
			__func__, cpu);
	}

	pthread_exit(NULL);
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		main
 * @BRIEF		main entry point
 * @RETURNS		0 on success
 *			-EINVAL in case of invalid argument
 *			-ECHILD in case of failure to fork
 * @param[in, out]	argc: shell input argument number
 * @param[in, out]	argv: shell input argument(s)
 * @DESCRIPTION		main entry point
 *//*------------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
	int i, ret, n, load;
	long int duration2;

	/*
	 * Register signal handler in order to be able to
	 * kill child process if user kills parent process
	 */
	signal(SIGTERM, (sighandler_t) sigterm_handler);

	printf("CPULOADGEN (REV %s)\n\n", CPULOADGEN_REVISION);

	cpu_count = (int) sysconf(_SC_NPROCESSORS_ONLN);
	if (cpu_count < 1) {
		fprintf(stderr, "cpuloadgen: could not determine CPU cores count!!! (%d)\n",
			cpu_count);
		return cpu_count;
	}
	dprintf("main: found %d CPU cores.\n", cpu_count);

	/* Allocate buffers */
	threads = malloc(cpu_count * sizeof(pthread_t));
	cpuloads = malloc(cpu_count * sizeof(int));
	if ((threads == NULL) || (cpuloads == NULL)) {
		fprintf(stderr, "cpuloadgen: could not allocate buffers!!!\n");
		return -ENOMEM;
	}
	/* Initialize variables */
	if (argc == 1) {
		/* No user arguments, use default */
		for (i = 0; i < cpu_count; i++) {
			threads[i] = -1;
			cpuloads[i] = 100;
		}
		duration = -1;
	} else {
		for (i = 0; i < cpu_count; i++) {
			threads[i] = -1;
			cpuloads[i] = -1;
		}
		duration = -1;

		/* Parse arguments */
		for (i = 1; i < argc; i++) {
			dprintf("main: argv[i]=%s\n", argv[i]);
			if (argv[i][0] == 'c') {
				ret = sscanf(argv[i], "cpu%d=%d", &n, &load);
				if ((ret != 2) ||
					((n < 0) || (n >= cpu_count)) ||
					((load < 1) || (load > 100)))
					return einval(argv[i]);
				if (cpuloads[n] != -1) {
					fprintf(stderr,
						"cpuloadgen: CPU%d was already assigned a load of %d!\n\n",
						n, cpuloads[n]);
					free_buffers();
					return -EINVAL;
				}
				cpuloads[n] = load;
				dprintf("Load assigned to CPU%d: %d%%\n",
					n, cpuloads[n]);
			} else if (argv[i][0] == 'd') {
				ret = sscanf(argv[i], "duration=%ld",
					&duration2);
				if ((ret != 1) || (duration2 < 1)) {
					return einval(argv[i]);
				}
				if (duration != -1) {
					fprintf(stderr,
						"cpuloadgen: duration was already set to %ld!\n\n",
						duration);
					free_buffers();
					return -EINVAL;
				}
				duration = duration2;
				dprintf("Duration of the load generation: %lds\n",
					duration);
			} else {
				return einval(argv[i]);
			}
		}
	}

	printf("Press CTRL+C to stop load generation at any time.\n\n");

	/* Start load generation on cores accordingly */
	for (i = 0; i < cpu_count; i++) {
		int cpu;
		if (cpuloads[i] == -1) {
			dprintf("main: no load to be generated on CPU%d\n", i);
			continue;
		}
		/*
		 * Why is a mutex needed here?
		 * There is a race condition between this loop which updates
		 * variable i and thread_loadgen() which also reads it.
		 * Hence locking a mutex here, and unlocking it in
		 * thread_loadgen() after the value was retrieved and saved.
		 */
		pthread_mutex_lock(&mutex1);
		cpu = i;
		ret = pthread_create(&threads[i], NULL, thread_loadgen, &cpu);
		if (ret != 0) {
			fprintf(stderr, "cpuloadgen: failed to fork %d! (%d)",
			i, ret);
			continue;
		}
	}

	for (i = 0; i < cpu_count; i++) {
		if (cpuloads[i] == -1) {
			continue;
		}
		pthread_join(threads[i], NULL);
	}

	free_buffers();

	printf("\ndone.\n\n");
	return 0;
}


/* ------------------------------------------------------------------------*//**
 * @FUNCTION		loadgen
 * @BRIEF		Programmable CPU load generator
 * @RETURNS		0 on success
 *			OMAPCONF_ERR_CPU
 *			OMAPCONF_ERR_ARG
 *			OMAPCONF_ERR_REG_ACCESS
 * @param[in]		cpu: target CPU core ID (loaded CPU core)
 * @param[in]		load: load to generate on that CPU ([1-100])
 * @param[in]		duration: how long this CPU core shall be loaded
 *				(in seconds)
 * @DESCRIPTION		Programmable CPU load generator. Use simple deadloops
 *			to generate load, and apply PWM (Pulse Width Modulation)
 *			principle on it to make average CPU load vary between
 *			0 and 100%
 *//*------------------------------------------------------------------------ */
void loadgen(unsigned int cpu, unsigned int load, unsigned int duration)
{
	double workload_start_time, workload_end_time;
	double idle_time_us;
	double loadgen_start_time_us, active_time_us;
	double total_time_us;
	struct timeval tv_cpuloadgen_start, tv_cpuloadgen;
#ifdef DEBUG
	struct timeval tv_idle_start, tv_idle_stop;
#endif
	struct timezone tz;
	double time_us;
#ifdef CPU_AFFINITY
	unsigned long mask;
	unsigned int len = sizeof(mask);
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	sched_setaffinity(0, len, &set);
	printf("Generating %3d%% load on CPU%d...\n", load, cpu);
#else
	printf("Generating %3d%% load...\n", load);
#endif

	gettimeofday(&tv_cpuloadgen_start, &tz);
	loadgen_start_time_us = ((double) tv_cpuloadgen_start.tv_sec
		+ ((double) tv_cpuloadgen_start.tv_usec * 1.0e-6));
	dprintf("%s(): CPU%d start time: %fus\n", __func__,
		cpu, loadgen_start_time_us);

	if (load != 100) {
		while (1) {
			/* Generate load (100%) */
			workload_start_time = dtime();
			workload(50000);
			workload_end_time = dtime();
			active_time_us =
				(workload_end_time - workload_start_time) * 1.0e6;
			dprintf("%s(): CPU%d running time: %dus\n", __func__,
				cpu, (unsigned int) active_time_us);

			/* Compute needed idle time */
			total_time_us =
				active_time_us * (100.0 / (double) (load + 1));
			dprintf("%s(): CPU%d total time: %dus\n", __func__, cpu,
				(unsigned int) total_time_us);
			idle_time_us = total_time_us - active_time_us;
			dprintf("%s(): CPU%d computed idle_time_us = %dus\n",
				__func__, cpu, (unsigned int) idle_time_us);

			/* Generate idle time */
			#ifdef DEBUG
			gettimeofday(&tv_idle_start, &tz);
			#endif
			usleep((unsigned int) (idle_time_us));
			#ifdef DEBUG
			gettimeofday(&tv_idle_stop, &tz);
			idle_time_us = 1.0e6 * (
				((double) tv_idle_stop.tv_sec +
				((double) tv_idle_stop.tv_usec * 1.0e-6))
				- ((double) tv_idle_start.tv_sec +
				((double) tv_idle_start.tv_usec * 1.0e-6)));
			dprintf("%s(): CPU%d effective idle time: %dus\n",
				__func__, cpu, (unsigned int) idle_time_us);
			dprintf("%s(): CPU%d effective CPU Load: %d%%\n",
				__func__, cpu,
				(unsigned int) (100.0 * (active_time_us /
				(active_time_us + idle_time_us))));
			#endif
			gettimeofday(&tv_cpuloadgen, &tz);
			time_us = ((double) tv_cpuloadgen.tv_sec
				+ ((double) tv_cpuloadgen.tv_usec * 1.0e-6));
			dprintf("%s(): CPU%d elapsed time: %fs\n",
				__func__, cpu,
				time_us - loadgen_start_time_us);
			if ((duration != 0) &&
				(time_us - loadgen_start_time_us) >= duration)
				break;
		}
	} else {
		while (1) {
			workload(1000000);
			gettimeofday(&tv_cpuloadgen, &tz);
			time_us = ((double) tv_cpuloadgen.tv_sec
				+ ((double) tv_cpuloadgen.tv_usec * 1.0e-6));
			dprintf("%s(): CPU%d elapsed time: %fs\n", __func__,
				cpu, time_us - loadgen_start_time_us);
			if ((duration != 0) &&
				(time_us - loadgen_start_time_us) >= duration)
				break;
		}
	}

	dprintf("Load Generation on CPU%d completed.\n", cpu);
}


void workload(unsigned int iterations)
{
    while (iterations-- > 0) {
        sqrt(rand());
    }
}

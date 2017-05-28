#include "timing.h"

using namespace std;
#ifdef ARMCC

#ifdef USECYCLES
#region cycles
double get_frequency(bool debug) {
#if (defined __linux__ || defined __blrts__) && \
    (defined __i386__ || defined __x86_64__ || defined __ia64__ || defined __PPC__) && \
    (defined __GNUC__ || defined __INTEL_COMPILER || defined __PATHSCALE__ || defined __xlC__)
	std::ifstream infile("/proc/cpuinfo");
	char     buffer[256], *colon;

	while (infile.good()) {
		infile.getline(buffer, 256);

		if (strncmp("bogoMIPS", buffer, 7) == 0 && (colon = strchr(buffer, ':')) != 0) {
			double freq = atof(colon + 2)*1e6;
			if (debug) cout << "Reported frequency (UNIX): " << (freq / 1e6) << " MHz" << endl;
			return freq;
		}
	}
	throw "Can't get frequency from proc/cpuinfo.";
#elif defined _WIN32
	LARGE_INTEGER frequency;
	if (::QueryPerformanceFrequency(&frequency) == FALSE)
		throw "Can't get frequency from QueryPerformanceFrequency.";

	//frequency is in kilo hertz
	if (debug) cout << "Reported frequency (Win32): " << frequency.QuadPart / 1e3 << " MHz" << endl;
	return (double)frequency.QuadPart * 1e3;
#endif
}
double diffToNanoseconds(unsigned int t1, unsigned int t2, double freq) {
	return (t2 - t1) / (freq) * 1e9;
}
unsigned int now() {
	unsigned int value;
	// Read CCNT Register
	asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));
	return value;

}
void init_perfcounters(int32_t do_reset, int32_t enable_divider)
{
	/* enable user-mode access to the performance counter*/
	asm("MCR p15, 0, %0, C9, C14, 0\n\t" :: "r"(1));

	/* disable counter overflow interrupts (just in case)*/
	asm("MCR p15, 0, %0, C9, C14, 2\n\t" :: "r"(0x8000000f));

	// in general enable all counters (including cycle counter)
	int32_t value = 1;

	// peform reset:
	if (do_reset)
	{
		value |= 2;     // reset all counters to zero.
		value |= 4;     // reset cycle counter to zero.
	}

	if (enable_divider)
		value |= 8;     // enable "by 64" divider for CCNT.

	value |= 16;

	// program the performance-counter control-register:
	asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(value));

	// enable all counters:
	asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));

	// clear overflows:
	asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));
}
#endregion
#else
double diffToNanoseconds(perftime_t t1, perftime_t t2, double freq) {

	return (double)((t2.tv_sec * 1e9 + t2.tv_nsec) - (t1.tv_sec * 1e9 + t1.tv_nsec));
}
perftime_t now() {
	struct timespec tp;
	//clock_gettime(CLOCK_REALTIME, &tp);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);    
    return tp;
}
#endif
#else
PerfClock::time_point now() {
	return PerfClock::now();
}

double diffToNanoseconds(PerfClock::time_point t1, PerfClock::time_point t2, double freq) {
	chrono::duration<double, nano> measurement = t2 - t1;
	return measurement.count();
}
#endif

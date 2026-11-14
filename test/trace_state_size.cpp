#include <stdio.h>
#include "../src/features/trace/kymo_trace.h"

int main(void)
{
    printf("%lu\n", (unsigned long)sizeof(KymoTraceRecorder));
    return 0;
}

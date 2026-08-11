/* Task 02: smallest cross-compile target. If this runs on the RG35XX (writes
 * hello.log), the toolchain -> cross-compile -> package -> run loop is proven.
 */
#include <stdio.h>
#include <time.h>

int main(void) {
    time_t t = time(NULL);
    printf("Hello from RG35XX! Elite port pipeline works.\n");
    printf("Time: %s", ctime(&t));
    return 0;
}

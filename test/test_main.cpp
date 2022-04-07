
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>

// This doesn't work because GFX has dependencies and pio test apparently doesn't support that.

int main(int argc, char** argv) {

    UNITY_BEGIN();    // IMPORTANT LINE!

    UNITY_END(); // stop unit testing
}



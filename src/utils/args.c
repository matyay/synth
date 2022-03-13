#include "args.h"

#include <stdlib.h>
#include <string.h>

// =============================================================================

char const* args(int argc, const char** argv, const char* header, const char* def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && i < (argc-1)) {
            if (argv[i+1][0] != '-') {
                return argv[i+1];
            }
        }
    }

    return def;
}

long argi(int argc, const char** argv, const char* header, long def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && i < (argc-1)) {
            return atol(argv[i+1]);
        }
    }

    return def;
}

long argh(int argc, const char** argv, const char* header, long def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && i < (argc-1)) {
            return strtol(argv[i+1], NULL, 16);
        }
    }

    return def;
}

float argf(int argc, const char** argv, const char* header, float def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && i < (argc-1)) {
            return atof(argv[i+1]);
        }
    }

    return def;
}

// =============================================================================

char const* argns(int argc, const char** argv, const char* header, int n, const char* def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && i < (argc-1)) {
            for(int j=0; j<=n; ++j) {
                int k = i+1+j;
                if (k >= argc || argv[k][0] == '-') {
                    break;
                }
                if (j == n) {
                    return argv[k];
                }
            }
        }
    }

    return def;
}

long argni(int argc, const char** argv, const char* header, int n, long def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && (i+1+n) < argc) {
            return atol(argv[i+1+n]);
        }
    }

    return def;
}

long argnh(int argc, const char** argv, const char* header, int n, long def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && (i+1+n) < argc) {
            return strtol(argv[i+1+n], NULL, 16);
        }
    }

    return def;
}

float argnf(int argc, const char** argv, const char* header, int n, float def) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header) && (i+1+n) < argc) {
            return atof(argv[i+1+n]);
        }
    }

    return def;
}

// =============================================================================

int argt(int argc, const char** argv, const char* header) {

    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], header)) {
            return 1;
        }
    }

    return 0;
}

// =============================================================================


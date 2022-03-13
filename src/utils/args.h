#ifndef ARGS_H
#define ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================

/// String argument
char const* args    (int argc, const char** argv, const char* header, const char* def);

/// Integer argument (decimal)
long        argi    (int argc, const char** argv, const char* header, long def);

/// Integer argument (hex)
long        argh    (int argc, const char** argv, const char* header, long def);

/// Float argument
float       argf    (int argc, const char** argv, const char* header, float def);


/// String argument (1 of N)
char const* argns   (int argc, const char** argv, const char* header, int n, const char* def);

/// Integer argument (decimal, 1 of N)
long        argni   (int argc, const char** argv, const char* header, int n, long def);

/// Integer argument (hex, 1 of N)
long        argnh   (int argc, const char** argv, const char* header, int n, long def);

/// Float argument (1 of N)
float       argnf   (int argc, const char** argv, const char* header, int n, float def);


/// Switch (no parameter) argument
int         argt    (int argc, const char** argv, const char* header);

// =============================================================================

#ifdef __cplusplus
}
#endif

#endif // ARGS_H

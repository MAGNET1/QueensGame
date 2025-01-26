#ifndef ASSERT_MSG_H
#define ASSERT_MSG_H

#include <assert.h>
#include <stdio.h>

#define ASSERT_MSG(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "Assertion failed: %s\n", #cond); \
            fprintf(stderr, fmt "\n", ##__VA_ARGS__); /* Use ##__VA_ARGS__ to remove the comma if no extra arguments exist */ \
            abort(); \
        } \
    } while (0)


#endif /* ASSERT_MSG_H */
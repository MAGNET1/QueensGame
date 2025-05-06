#ifndef UUIDV7_H
#define UUIDV7_H

#include <basic_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <debug_print.h>

void UUIDv7_Generate(uint8 uuid[16]);
void UUIDv7_Print(const uint8 uuid[16]);

#endif /* UUIDV7_H */

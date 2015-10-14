#pragma once
#include "hash.h"

#define _(str) locale_str(HASH_DJB2(str))

void locale_init(void);
void locale_deinit(void);

char *locale_str(int hashval);
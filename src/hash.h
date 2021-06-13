#pragma once

#ifndef HASH_H
#define HASH_H

unsigned long hash(const char* str, unsigned long len);
unsigned long combine(unsigned long hash_a, unsigned long hash_b);

#endif // !HASH_H
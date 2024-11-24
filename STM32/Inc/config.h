#pragma once

#include <stdbool.h>

bool cfg_init(bool (*validate_cb)(void const*), unsigned obj_sz, void* obj_ptr);
void cfg_save(unsigned obj_sz, void const* obj_ptr);

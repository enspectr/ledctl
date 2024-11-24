#include "main.h"
#include "config.h"
#include "debug.h"
#include <string.h>

#define CFG_PAGE   15 // The last page of 32k flash
#define CFG_ADDR   (FLASH_BASE + FLASH_PAGE_SIZE * CFG_PAGE)
#define CFG_DWORDS (FLASH_PAGE_SIZE/sizeof(uint64_t))

struct config_page {
	uint64_t dwords[CFG_DWORDS];
};

// Saved config
const __root __no_init struct config_page cfg_page @ CFG_ADDR;
unsigned cfg_tail;

static inline void cfg_unlock(void)
{
	HAL_StatusTypeDef const rc = HAL_FLASH_Unlock();
	BUG_ON(rc != HAL_OK);
}

static inline void cfg_lock(void)
{
	HAL_StatusTypeDef const rc = HAL_FLASH_Lock();
	BUG_ON(rc != HAL_OK);
}

static void cfg_erase(void)
{
	uint32_t bad_page;
	FLASH_EraseInitTypeDef erase_param = {
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.Page = CFG_PAGE,
		.NbPages = 1
	};
	HAL_StatusTypeDef const rc = HAL_FLASHEx_Erase(&erase_param, &bad_page);
	BUG_ON(rc != HAL_OK);
	cfg_tail = 0;
}

static void cfg_store(unsigned obj_sz, void const* obj_ptr)
{
	unsigned const dw_sz = obj_sz / sizeof(uint64_t);
	uint64_t const* dw_ptr = (uint64_t const*)obj_ptr;
	unsigned const end = cfg_tail + dw_sz;
	BUG_ON(end > CFG_DWORDS);
	for (unsigned i = cfg_tail; i < end; ++i, ++dw_ptr) {
		HAL_StatusTypeDef const rc = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, CFG_ADDR + i * sizeof(uint64_t), *dw_ptr);
		BUG_ON(rc != HAL_OK);
	}
	cfg_tail = end;
}

bool cfg_init(bool (*validate_cb)(void const*), unsigned obj_sz, void* obj_ptr)
{
	BUG_ON(!obj_sz);
	BUG_ON(obj_sz % sizeof(uint64_t));
	BUG_ON(obj_sz > FLASH_PAGE_SIZE);
	unsigned const dw_sz = obj_sz / sizeof(uint64_t);
	for (unsigned i = 0, e = dw_sz; e <= CFG_DWORDS; i = e, e += dw_sz) {
		if (!validate_cb(&cfg_page.dwords[i]))
			break;
		cfg_tail = e;
	}
	if (cfg_tail) {
		for (unsigned i = cfg_tail; i < CFG_DWORDS; ++i) {
			if (~cfg_page.dwords[i]) {
				cfg_tail = 0;
				break;
			}
		}
	}
	if (cfg_tail) {
		memcpy(obj_ptr, &cfg_page.dwords[cfg_tail - dw_sz], obj_sz);
		return true;
	}

	cfg_unlock();
	cfg_erase();
	cfg_store(obj_sz, obj_ptr);
	cfg_lock();
	return false;
}

void cfg_save(unsigned obj_sz, void const* obj_ptr)
{
	unsigned const dw_sz = obj_sz / sizeof(uint64_t);	
	cfg_unlock();
	if (cfg_tail + dw_sz > CFG_DWORDS)
		cfg_erase();
	cfg_store(obj_sz, obj_ptr);
	cfg_lock();
}


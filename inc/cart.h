#pragma once

// Cartridge Header
#define CART_HEADER_START 0x100
#define CART_HEADER_END 0x14F
#define CART_HEADER_SIZE CART_HEADER_END - CART_HEADER_START

// Cartridge Header Data Locations
#define CART_HEADER_TITLE 0x134
#define CART_HEADER_GBC_FLAG 0x143
#define CART_HEADER_TYPE 0x147
#define CART_HEADER_ROM_SIZE 0x148
#define CART_HEADER_RAM_SIZE 0x149


bool load_rom(GameBoy *gb, const char *path);
void print_cart_info(GameBoy *gb);
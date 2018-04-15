#pragma once

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define MAIN_WINDOW_TITLE "LXGBC"

// LCDC: LCD Control Register 
#define LCDC 0xFF40
#define LCDC_LCD_ENABLE 7 // LCD Display Enable
#define LCDC_WINDOW_TILE_MAP 6 // Window Tile Map Display Select
#define LCDC_WINDOW_DISPLAY 5 // Window Display Enable
#define LCDC_BG_WINDOW_TILE_DATA 4 // BG & Window Tile Data Select
#define LCDC_BG_TILE_MAP 3 // BG Tile Map Display Select
#define LCDC_OBJ_SIZE 2 // OBJ (Sprite) Size 
#define LCDC_OBJ_DISPLAY 1 // OBJ (Sprite) Display Enable
#define LCDC_BG_DISPLAY 0 // BG Display

// STAT: LCDC Status Register
#define STAT 0xFF41
#define STAT_LYC_LY_COINCID 6 // LYC=LY Coincidence Interrupt
#define STAT_OAM 5 // Mode 2 OAM Interrupt
#define STAT_VBLANK 4 // Mode 1 V-Blank Interrupt
#define STAT_HBLANK 3 // Mode 0 H-Blank Interrupt
#define STAT_COINCID_FLAG 2 // Coincidence Flag
#define STAT_MODE0 1 // Mode Flag 1/2 
#define STAT_MODE1 0 // Mode Flag 2/2

// SCY: Scroll Y
#define SCY 0xFF42

// SCX: Scroll X
#define SCX 0xFF43

// LY: LCDC Y-Coordinate
#define LY 0xFF44

// LYC: LY Compare
#define LYC 0xFF45

// WY: Window Position Y
#define WY 0xFF4A

// WX: Window Position X minus 7
#define WX 0xFF4B

// BGP: BG Palette Data (NON CGB)
#define BGP 0xFF47

// 0BP0: Object Palette 0 Data (NON CGB)
#define OBP0 0xFF48

// 0BP1: Object Palette 1 Data (NON CGB)
#define OBP1 0xFF49

// 
#define DMA 0xFF46


#define REG_BCPS 0xFF68
#define REG_BCPD 0xFF69
#define REG_OCPS 0xFF6A
#define REG_OCPD 0xFF6B
#define VBK 0xFF4F
#define HDMA1 0xFF51
#define HDMA2 0xFF52
#define HDMA3 0xFF53
#define HDMA4 0xFF54
#define HDMA5 0xFF55

// GameBoy Monochrome Palette
#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 255
#define DGREY 192, 192, 192, 255
#define LGREY 96, 96, 96, 255

void init_ppu(gbc_ppu *, const char scale);
void ppu_do_clock(gbc_system *);
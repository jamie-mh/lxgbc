#define VRAM_SIZE 8192
#define EXTRAM_BANK_SIZE 8192
#define WRAM_BANK_SIZE 4096
#define OAM_SIZE 159
#define IO_SIZE 128
#define HRAM_SIZE 128

#define WRAM_BANK_COUNT 8


// Sound Registers
// TODO: Implement

// Joypad Register
#define JOYP 0xFF00

// Timer and Divider Registers
#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07

// GBC Only Registers
#define KEY1 0xFF4D
#define SVBK 0xFF70

// Interrupt Registers
#define REG_IE 0xFFFF
#define REG_IF 0xFF0F

typedef struct gbc_ram {
    unsigned char *rom00; // 16KB ROM Bank
    unsigned char *romNN; // 16KB Switchable ROM Bank
    unsigned char *vram; // 8KB Video RAM
    unsigned char *extram; // 8KB External Ram (cartridge)
    unsigned char *wram00; // 4KB Work RAM bank 0
    unsigned char *wramNN; // 4KB Work RAM bank 1-7 (switchable) 
    unsigned char *oam; // 1.59KB Sprite Attribute Table
    unsigned char *io; // 128B IO Ports
    unsigned char *hram; // 128B High RAM
    unsigned char *ier; // 1B Interrupt Enable Register

    unsigned char **wram_banks; // 8x4KB WRAM Banks (CGB Only)
} gbc_ram;

void init_ram(gbc_ram *);
static unsigned char *get_memory_location(gbc_ram *, unsigned short *);

unsigned char read_byte(gbc_ram *, const unsigned short);
unsigned short read_short(gbc_ram *, const unsigned short);

void write_byte(gbc_ram *, const unsigned short, const unsigned char);
void write_short(gbc_ram *, const unsigned short, const unsigned short);

void set_register(gbc_ram *, const unsigned short, const unsigned char, const unsigned char);
unsigned char read_register(gbc_ram *, const unsigned short, const unsigned char);

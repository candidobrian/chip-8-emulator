#include "SDL2/SDL.h"
#include "config.h"
#include "platform.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Memory: 4096 bytes
uint8_t memory[C8_MEM_SIZE] = {0};

// Registers:
// 16 8-bit data registers (V0-VF),
// index register (I),
// program counter (PC),
// and stack pointer (SP)
uint8_t V[C8_TOTAL_DATA_REGISTERS] = {0};
uint16_t I = 0;
uint16_t PC = 0;
uint8_t SP = 0;

// Stack: 16 16-bit values
uint16_t stack[C8_TOTAL_STACK_DEPTH] = {0};

// Screen: 64x32 pixels
bool screen[C8_HEIGHT][C8_WIDTH] = {0};

// Keyboard: 16 keys
bool keyboard[C8_TOTAL_KEYS] = {0};
char const keymap[C8_TOTAL_KEYS] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,
    SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f};

// Timers
uint8_t delay_timer = 0;
uint8_t sound_timer = 0;

// Sprites: 8-bit by 5-bytes
uint8_t const sprites[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initialise(void) {
  memset(memory, 0, sizeof(memory));
  memcpy(memory, sprites, sizeof(sprites));
  memset(V, 0, sizeof(V));
  I = 0;
  PC = C8_PROGRAM_LOAD_ADDR;
  SP = 0;
  memset(stack, 0, sizeof(stack));
  memset(screen, 0, sizeof(screen));
  memset(keyboard, false, sizeof(keyboard));
  delay_timer = 0;
  sound_timer = 0;
  srand(time(NULL));
}

void load_rom(char const* filename) {
  FILE* rom = fopen(filename, "rb");
  if (rom == NULL) {
    perror("Error opening ROM file");
    exit(EXIT_FAILURE);
  }
  size_t bytes_read = fread(memory + C8_PROGRAM_LOAD_ADDR, 1,
                            C8_MEM_SIZE - C8_PROGRAM_LOAD_ADDR, rom);
  fclose(rom);
  printf("Loaded %zu bytes from %s\n", bytes_read, filename);
}

int get_key_index(char const key) {
  for (int i = 0; i < C8_TOTAL_KEYS; ++i) {
    if (keymap[i] == key) {
      return i;
    }
  }
  return -1;
}

void emulate_cycle(void) {
  uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
  PC += 2;

  uint8_t x = (opcode >> 8) & 0x000F;
  uint8_t y = (opcode >> 4) & 0x000F;
  uint8_t n = opcode & 0x000F;
  uint8_t kk = opcode & 0x00FF;
  uint16_t nnn = opcode & 0x0FFF;

  switch (opcode >> 12) {
  case 0x0:
    switch (opcode & 0x00FF) {
    case 0x00E0: // CLS
      memset(screen, 0, sizeof(screen));
      break;
    case 0x00EE: // RET
      if (SP > 0) {
        SP--;
        PC = stack[SP];
      }
      break;
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
    }
    break;
  case 0x1: // JP addr
    PC = nnn;
    break;
  case 0x2: // CALL addr
    if (SP < C8_TOTAL_STACK_DEPTH) {
      stack[SP] = PC;
      SP++;
      PC = nnn;
    }
    break;
  case 0x3: // SE Vx, byte
    if (V[x] == kk)
      PC += 2;
    break;
  case 0x4: // SNE Vx, byte
    if (V[x] != kk)
      PC += 2;
    break;
  case 0x5: // SE Vx, Vy
    if (V[x] == V[y])
      PC += 2;
    break;
  case 0x6: // LD Vx, byte
    V[x] = kk;
    break;
  case 0x7: // ADD Vx, byte
    V[x] += kk;
    break;
  case 0x8:
    switch (opcode & 0x000F) {
    case 0x0: // LD Vx, Vy
      V[x] = V[y];
      break;
    case 0x1: // OR Vx, Vy
      V[x] |= V[y];
      break;
    case 0x2: // AND Vx, Vy
      V[x] &= V[y];
      break;
    case 0x3: // XOR Vx, Vy
      V[x] ^= V[y];
      break;
    case 0x4: // ADD Vx, Vy
      if (V[y] > (0xFF - V[x]))
        V[0xF] = 1;
      else
        V[0xF] = 0;
      V[x] += V[y];
      break;
    case 0x5: // SUB Vx, Vy
      if (V[y] > V[x])
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[x] -= V[y];
      break;
    case 0x6: // SHR Vx {, Vy}
      V[0xF] = V[x] & 0x1;
      V[x] >>= 1;
      break;
    case 0x7: // SUBN Vx, Vy
      if (V[x] > V[y])
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[x] = V[y] - V[x];
      break;
    case 0xE: // SHL Vx {, Vy}
      V[0xF] = (V[x] >> 7) & 0x1;
      V[x] <<= 1;
      break;
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
    }
    break;
  case 0x9: // SNE Vx, Vy
    if (V[x] != V[y])
      PC += 2;
    break;
  case 0xA: // LD I, addr
    I = nnn;
    break;
  case 0xB: // JP V0, addr
    PC = nnn + V[0x0];
    break;
  case 0xC: // RND Vx, byte
    V[x] = (rand() % 256) & kk;
    break;
  case 0xD: // DRW Vx, Vy, nibble
  {
    V[0xF] = 0;
    for (int row = 0; row < n; ++row) {
      uint8_t sprite_byte = memory[I + row];
      for (int col = 0; col < 8; ++col) {
        if ((sprite_byte & (0x80 >> col)) != 0) {
          int pixel_x = (V[x] + col) % C8_WIDTH;
          int pixel_y = (V[y] + row) % C8_HEIGHT;
          if (screen[pixel_y][pixel_x]) {
            V[0xF] = 1;
          }
          screen[pixel_y][pixel_x] ^= 1;
        }
      }
    }
    break;
  }
  case 0xE:
    switch (opcode & 0x00FF) {
    case 0x9E: // SKP Vx
      if (keyboard[V[x]])
        PC += 2;
      break;
    case 0xA1: // SKNP Vx
      if (!keyboard[V[x]])
        PC += 2;
      break;
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
    }
    break;
  case 0xF:
    switch (opcode & 0x00FF) {
    case 0x07: // LD Vx, DT
      V[x] = delay_timer;
      break;
    case 0x0A: // LD Vx, K
    {
      bool key_pressed = false;
      while (!key_pressed) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
          if (event.type == SDL_KEYDOWN) {
            char key = event.key.keysym.sym;
            int key_index = get_key_index(key);
            if (key_index != -1) {
              V[x] = key_index;
              keyboard[key_index] = true;
              key_pressed = true;
            }
          } else if (event.type == SDL_KEYUP) {
            char key = event.key.keysym.sym;
            int key_index = get_key_index(key);
            if (key_index != -1 && key_index < C8_TOTAL_KEYS) {
              keyboard[key_index] = false;
            }
          }
        }
        sleep_ms(1); // Small delay for pressing keys
      }
      break;
    }
    case 0x15: // LD DT, Vx
      delay_timer = V[x];
      break;
    case 0x18: // LD ST, Vx
      sound_timer = V[x];
      break;
    case 0x1E: // ADD I, Vx
      I += V[x];
      break;
    case 0x29: // LD F, Vx
      I = V[x] * C8_DEFAULT_SPRITE_HEIGHT;
      break;
    case 0x33: // LD B, Vx
      memory[I] = V[x] / 100;
      memory[I + 1] = (V[x] / 10) % 10;
      memory[I + 2] = V[x] % 10;
      break;
    case 0x55: // LD [I], Vx
      for (int i = 0; i <= x; ++i) {
        memory[I + i] = V[i];
      }
      break;
    case 0x65: // LD Vx, [I]
      for (int i = 0; i <= x; ++i) {
        V[i] = memory[I + i];
      }
      break;
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
    }
    break;
  default:
    printf("Unknown opcode: 0x%X\n", opcode);
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
    return EXIT_SUCCESS;
  }

  initialise();
  load_rom(argv[1]);

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window* window =
      SDL_CreateWindow(EMULATOR_WIN_TITLE, SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, C8_WIDTH * C8_WIN_MULTIPLIER,
                       C8_HEIGHT * C8_WIN_MULTIPLIER, SDL_WINDOW_SHOWN);

  if (!window) {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  bool quit = false;
  SDL_Event event;

  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = true;
      } else if (event.type == SDL_KEYDOWN) {
        char key = event.key.keysym.sym;
        int key_index = get_key_index(key);
        if (key_index != -1 && key_index < C8_TOTAL_KEYS) {
          keyboard[key_index] = true;
        }
      } else if (event.type == SDL_KEYUP) {
        char key = event.key.keysym.sym;
        int key_index = get_key_index(key);
        if (key_index != -1 && key_index < C8_TOTAL_KEYS) {
          keyboard[key_index] = false;
        }
      }
    }

    // Allow four instructions per frame. Adjusts responsiveness
    for (size_t i = 0; i < 4; ++i) {
      emulate_cycle();
    }

    if (delay_timer > 0)
      delay_timer--;
    if (sound_timer > 0)
      sound_timer--;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < C8_HEIGHT; ++y) {
      for (int x = 0; x < C8_WIDTH; ++x) {
        if (screen[y][x]) {
          SDL_Rect rect = {x * C8_WIN_MULTIPLIER, y * C8_WIN_MULTIPLIER,
                           C8_WIN_MULTIPLIER, C8_WIN_MULTIPLIER};
          SDL_RenderFillRect(renderer, &rect);
        }
      }
    }

    SDL_RenderPresent(renderer);

    sleep_ms(16); // Approximately 60 Hz
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}

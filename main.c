#include "logger.c"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// memory
uint16_t memory[UINT16_MAX];

// register
enum {
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC, /* program counter */
  R_COND,
  R_COUNT
};
uint16_t reg[R_COUNT];

// instructions
enum {
  OP_BR = 0, /* branch */
  OP_ADD,    /* add  */
  OP_LD,     /* load */
  OP_ST,     /* store */
  OP_JSR,    /* jump register */
  OP_AND,    /* bitwise and */
  OP_LDR,    /* load register */
  OP_STR,    /* store register */
  OP_RTI,    /* unused */
  OP_NOT,    /* bitwise not */
  OP_LDI,    /* load indirect */
  OP_STI,    /* store indirect */
  OP_JMP,    /* jump */
  OP_RES,    /* reserved (unused) */
  OP_LEA,    /* load effective address */
  OP_TRAP    /* execute trap */
};

// Condition flags
enum {
  FL_POS = 1 << 0, /* P */
  FL_ZRO = 1 << 1, /* Z */
  FL_NEG = 1 << 2, /* N */
};

// debug util
void util_log_reg() {
  for (uint8_t i = 0; i < R_COUNT; i++) {
    if (i == R_PC) {
      vm_logger_dev("reg[R_PC]: 0x%x\n", reg[R_PC]);
      continue;
    }

    if (i == R_COND) {
      vm_logger_dev("reg[R_COND]: 0x%x\n", reg[R_COND]);
      continue;
    }

    vm_logger_dev("reg[%d]: 0x%x\n", i, reg[i]);
  }
}

void util_log_binary(uint16_t num) {
  for (uint8_t i = 0; i < 16; i++) {
    if (i % 4 == 0) {
      printf(" ");
    }
    printf("%d", (num & (1 << (15 - i))) >> (15 - i));
  }
  printf("\n");
}

void util_load_test_program() {
  vm_logger_dev("load test program:\n         ADD R0, R0, -1\n");
  reg[R_R0] = 2;
  memory[reg[R_PC]] = 0b0001000000111111;
  memory[reg[R_PC] + 1] = -1;
}

void init_vm();
void op_add(uint16_t inst);
void op_and(uint16_t inst);
void op_br(uint16_t inst);
void op_jmp(uint16_t inst);
uint16_t _5_bits_sign_extend(uint8_t num);
uint16_t _9_bits_sign_extend(uint16_t num);
void update_cond_reg(uint16_t reg);

int main(int argc, char *argv[]) {
  vm_logger_dev("startup\n");

  vm_logger_dev("argc: %d\n", argc);
  for (uint8_t i = 0; i < argc; i++) {
    vm_logger_dev("argv[%d]: %s\n", i, argv[i]);
  }

  vm_logger_dev("initialize memory and register\n");
  init_vm();
  util_load_test_program();

  vm_logger_dev("start program\n");
  uint8_t running = 1;
  while (running) {
    uint16_t inst = memory[reg[R_PC]++];
    uint8_t op = inst >> 12;
    switch (op) {
    case OP_ADD: {
      op_add(inst);
      break;
    };
    case OP_AND: {
      op_and(inst);
      break;
    };
    case OP_BR: {
      op_jmp(inst);
      break;
    };
    case OP_JMP: {
      op_jmp(inst);
      break;
    };
    default:
      vm_logger_panic("OP: 0x%x not supported\n", op);
      running = 0;
    }
  }

  util_log_reg();
  assert(reg[R_R0] == 0b1);
  vm_logger_dev("shutdown\n");
  return EXIT_SUCCESS;
}

void init_vm() {
  memset(memory, 0, sizeof(memory));
  memset(reg, 0, sizeof(reg));

  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;
}

void op_add(uint16_t inst) {
  uint8_t destination_reg = (inst & 0b0000111000000000) >> 9;
  uint8_t source1_reg = (inst & 0b0000000111000000) >> 6;

  uint8_t mod = (inst & 0b0000000000100000) >> 5;
  if (mod == 1) {
    // immediate mode
    uint16_t immediate_value_5bits =
        _5_bits_sign_extend(inst & 0b0000000000011111);
    reg[destination_reg] = reg[source1_reg] + immediate_value_5bits;
    return;
  } else /* if (mod == 0) */ {
    // register mode
    uint8_t source2_reg = inst & 0b000000000000111;
    reg[destination_reg] = reg[source1_reg] + reg[source2_reg];
    return;
  }

  update_cond_reg(destination_reg);
}

void op_and(uint16_t instruction) {
  uint8_t destination_reg = (instruction & 0b0000111000000000) >> 9;
  uint8_t source1_reg = (instruction & 0b0000000111000000) >> 6;

  uint8_t mod = (instruction & 0b0000000000100000) >> 5;
  if (mod == 1) {
    // immediate mode
    uint16_t immediate_value_5bits =
        _5_bits_sign_extend(instruction & 0b0000000000011111);
    reg[destination_reg] = reg[source1_reg] & immediate_value_5bits;
    return;
  } else /* if (mod == 0) */ {
    // register mode
    uint8_t source2_reg = instruction & 0b000000000000111;
    reg[destination_reg] = reg[source1_reg] & reg[source2_reg];
    return;
  }

  update_cond_reg(destination_reg);
}

void op_br(uint16_t inst) {
  uint16_t pc_offset = _9_bits_sign_extend(inst & 0b0000000111111111);
  uint8_t test_fl_pos = (inst & 0b0000001000000000) >> 9;
  uint8_t test_fl_zro = (inst & 0b0000010000000000) >> 10;
  uint8_t test_fl_neg = (inst & 0b0000100000000000) >> 11;
  if ((test_fl_pos == 1 && reg[R_COND] == FL_POS) ||
      (test_fl_neg == 1 && reg[R_COND] == FL_NEG) ||
      (test_fl_zro == 1 && reg[R_COND] == FL_ZRO)) {
    reg[R_PC] += _9_bits_sign_extend(pc_offset & 0b0000000000000000);
  }
}

void op_jmp(uint16_t inst) {
  uint8_t jmp_to_reg = (inst & 0b0000000111000000) >> 6;
  reg[R_PC] = reg[jmp_to_reg];
}

uint16_t _5_bits_sign_extend(uint8_t num) {
  if ((num & 0b00010000) >> 4) {
    return num | 0b1111111111100000;
  }

  return num;
}

uint16_t _9_bits_sign_extend(uint16_t num) {
  if ((num & 0b00010000) >> 8) {
    return num | 0b1111111000000000;
  }

  return num;
}
void update_cond_reg(uint16_t modified_reg) {
  if (reg[modified_reg] == 0) {
    reg[R_COND] = FL_ZRO;
    return;
  }

  if (reg[modified_reg] >> 15 == 1) {
    reg[R_COND] = FL_NEG;
    return;
  }

  reg[R_COND] = FL_POS;
}

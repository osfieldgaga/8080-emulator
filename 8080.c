#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "constants.h"

typedef struct ConditionalCodes
{
    uint8_t s : 1;
    uint8_t z : 1;
    uint8_t ac : 1;
    uint8_t p : 1;
    uint8_t cy : 1;
} ConditionalCodes;

typedef struct State8080
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t sp;
    uint16_t pc;

    // memory is ROM + RAM
    // 8kb of ROM ($0000 to $1fff) and 8kb or RAM ($2000 to $3fff)
    uint8_t *memory;

    struct ConditionalCodes cc;
    uint8_t int_enabled; // interrupt enable
    unsigned int cycles;
    // uint8_t *bus;
} State8080;

void InitializeRegisters(State8080 *state)
{
    state->a = 0x00;
    state->b = 0x00;
    state->c = 0x00;
    state->d = 0x00;
    state->e = 0x00;
    state->h = 0x00;
    state->l = 0x00;
    state->sp = 0x00;
    state->pc = 0x00;
    state->int_enabled = 0x00;
}

void InitializeMemory(State8080 *state)
{
    if (!FOR_CPUDIAG)
    {
        // allocate memory for 16KB (8KB ROM + 8KB RAM)
        state->memory = (uint8_t *)malloc(0x4000);
    }
    else
    {
        state->memory = (uint8_t *)malloc(0x1453);
    }
}

int stack_size = 0;
int stack[100];

void ShowState(State8080 *state)
{
    // uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (state->cc.ac << 4) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);

    printf("\nSP: %02x\n", state->sp);
    printf("PC: %02x\n", state->pc);
    printf("A: %02x\n", state->a);
    // printf("F: %02x\n", flags);
    printf("B: %02x\n", state->b);
    printf("C: %02x\n", state->c);
    printf("D: %02x\n", state->d);
    printf("E: %02x\n", state->e);
    printf("H: %02x\n", state->h);
    printf("L: %02x\n", state->l);
    printf("Interrupt: %02x\n", state->int_enabled);

    // printf("Cycles: %d\n\n", state->cycles);
    // printf("CALL/RET content: %02x%02x\n", state->memory[state->sp + 1], state->memory[state->sp]);

    printf("S  Z  P  C\n");
    printf("%x  %x  %x  %x\n\n", state->cc.s, state->cc.z, state->cc.p, state->cc.cy);

    // printf("Stack\n");
    //     int i;
    //     for (i = 0; i < stack_size; i++) {
    //         printf("Stack[%d] = %04x\n", i, stack[i]);
    //     }
}

void SetFlags(State8080 *state, uint16_t ops_result)
{
    state->cc.z = ((ops_result & 0xff) == 0) ? 1 : 0;
    state->cc.s = (ops_result >> 7 & 0x01 == 1) ? 1 : 0;

    int i, count = 0;
    // printf("Parity check: 0b");
    for (i = 0; i < 8; i++)
    {

        if ((ops_result >> i & 0x01) == 1)
        {
            count++;
            // printf("1");
        } else {
            // printf("0");
        }
    }
        // printf("\n");
    state->cc.p = (count % 2 == 0) ? 1 : 0;

    state->cc.ac = ((((ops_result & 0xF) - 1) & 0x10) == 0x10) ? 1 : 0;

    // ops_result is assumed 32 bits, in the even the 16th bit is set, it means there was a carry.
    // mask sum with 0b00000000000000010000000000000000 (17th bit is 1, rest is 0) and check
    // if the mask results in 0x10000, which would mean that bit in on and that a carry happened
    // state->cc.cy = (ops_result & 0x10000 == 0x10000) ? 1 : 0;

    // Carry flag: set if the result is greater than 8 bits
    state->cc.cy = (ops_result > 0xFF) ? 1 : 0;
}

void UnimplementedInstruction(State8080 *state)
{
    uint8_t *opcode = &state->memory[state->pc];
    printf("Instruction %02x not implemented yet.\n", *opcode);

    printf("\n\nState on unimplemented\n");

    if (LOGS_CPU)
    {
        ShowState(state);
    }

    exit(1);
}

int Emulate8080(State8080 *state)
{
    int opbytes = 1;
    uint8_t *opcode = &state->memory[state->pc];
    uint16_t address;

    if (LOGS_CPU)
    {
        printf("Executing opcode: %02x, PC is %02x\n", *opcode, state->pc);
    }
    switch (*opcode)
    {
    case 0x00:
        opbytes = 1;
        state->cycles += 4;
        break;

    case 0x01:
        // 0x01	LXI B,D16	3		B <- byte 3, C <- byte 2
        state->b = (state->memory[state->pc + 2]);
        state->c = (state->memory[state->pc + 1]);
        // printf("Changed BC to %02x%02x\n", state->b, state->c);
        opbytes = 3;
        state->cycles += 10;
        break;
    case 0x02:
        // 0x02	STAX B	1		(BC) <- A
        {
            uint16_t bc = (state->b << 8) + (state->c);
            state->memory[bc] = state->a;

            state->cycles += 7;
            opbytes = 1;
            break;
        }
    case 0x03:
        // 0x03	INX B	1		BC <- BC+1
        {
            uint16_t bc_temp = (state->b << 8) + (state->c);
            bc_temp += 1;

            state->c = bc_temp & 0xFF;
            state->b = bc_temp >> 8 & 0xFF;

            opbytes = 1;
            state->cycles += 5;
            break;
        }
    case 0x04:
        // 0x04	INR B	1	Z, S, P, AC	B <- B+1
        state->b += 1;

        SetFlags(state, state->b);

        // printf("%02x", (state->b & 0xF));

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x05:
        // 0x05	DCR B	1	Z, S, P, AC	B <- B-1
        state->b -= 1;

        SetFlags(state, state->b);

        // printf("%02x", (state->b & 0xF));

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x06:
        // 0x06	MVI B, D8	2		B <- byte 2
        state->b = (state->memory[state->pc + 1]);
        // printf("Moved into B: %02x\n", state->b);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x07:
        // 0x07	RLC	1	CY	A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
        {
            uint8_t rlc_temp = (state->a << 1) + ((state->a >> 7) & 0x01);

            state->cc.cy = (state->a >> 7) & 0x01;
            state->a = rlc_temp;

            opbytes = 1;
            state->cycles += 4;
            break;
        }
    case 0x08:
        // 0x08	-
        opbytes = 1;
        break;
    case 0x09:
        // 0x09	DAD B	1	CY	HL = HL + BC
        {
            uint16_t hl_temp = (state->h << 8) + (state->l);
            uint16_t bc_temp = (state->b << 8) + (state->c);

            uint32_t sum = hl_temp + bc_temp;

            state->l = sum & 0xFF;
            state->h = sum >> 8 & 0xFF;

            // sum is 32 bits, in the even the 16th bit is set, it means there was a carry
            // mask sum with 0b00000000000000010000000000000000 (17th bit is 1, rest is 0) and check
            // if the mask results in 0x10000, which would mean that bit in on and that a carry happened
            state->cc.cy = (sum > 0xff) ? 1 : 0;

            state->cycles += 3;
            opbytes = 1;
            break;
        }
    case 0x0a:
    {
        uint16_t bc = (state->b << 8) + (state->c);
        state->a = state->memory[bc];

        state->cycles += 7;
        opbytes = 1;
        break;
    }
    case 0x0b:
        // 0x0b	DCX B	1		BC = BC-1
        {
            uint16_t bc_temp = (state->b << 8) + (state->c);
            bc_temp -= 1;

            state->c = bc_temp & 0xFF;
            state->b = bc_temp >> 8 & 0xFF;
            state->cycles += 5;
            opbytes = 1;
            break;
        }
    case 0x0c:
        // 0x0c	INR C	1	Z, S, P, AC	C <- C+1
        state->c += 1;

        SetFlags(state, state->c);

        // printf("%02x", (state->c & 0xF));

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x0d:
        // 0x0d	DCR C	1	Z, S, P, AC	C <-C-1
        state->c -= 1;

        SetFlags(state, state->c);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x0e:
        // 0x0e	MVI C,D8	2		C <- byte 2
        state->c = (state->memory[state->pc + 1]);
        // printf("Moved into C: %02x\n", state->c);
        opbytes = 2;
        state->cycles += 7;
        break;

    case 0x0f:
    {
        // 0x0f	RRC	1	CY	A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
        uint8_t a_temp = state->a;
        state->a = (state->a >> 1);
        state->a = ((a_temp & 0x01) << 7) | (state->a & 0x7F);
        state->cc.cy = (a_temp & 0x01);

        opbytes = 1;
        state->cycles += 1;
        break;
    }
    case 0x10:
        // 0x10	-
        opbytes = 1;
        break;
    case 0x11:
        // 0x11	LXI D,D16	3		D <- byte 3, E <- byte 2

        state->d = (state->memory[state->pc + 2]);
        state->e = (state->memory[state->pc + 1]);
        // printf("Changed HL to %02x%02x\n", state->h, state->l);
        state->cycles += 10;
        opbytes = 3;
        break;
    case 0x12:
        // 0x12	STAX D	1		(DE) <- A
        // store whatever is in A in memory with address [whatever is contained in DE]
        state->memory[(state->d << 8) + (state->e)] = state->a;
        opbytes = 1;
        state->cycles += 7;
        break;

    case 0x13:
        // INX D	1		DE <- DE + 1
        {
            uint16_t de_temp = (state->d << 8) + (state->e);
            de_temp += 1;

            state->e = de_temp & 0xFF;
            state->d = de_temp >> 8 & 0xFF;

            opbytes = 1;
            state->cycles += 5;
            break;
        }
    case 0x14:
        // 	0x14	INR D	1	Z, S, P, AC	D <- D+1
        state->d += 1;

        SetFlags(state, state->d);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x15:
        // 0x15	DCR D	1	Z, S, P, AC	D <- D-1
        state->d -= 1;

        SetFlags(state, state->d);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x16:
        // 0x16	MVI D, D8	2		D <- byte 2
        state->d = (state->memory[state->pc + 1]);
        // printf("Moved into D: %02x\n", state->d);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x17:
        // 0x17	RAL	1	CY	A = A << 1; bit 0 = prev CY; CY = prev bit 7
        {
            uint8_t ral_temp = (state->a << 1) + state->cc.cy;

            state->cc.cy = (state->a >> 7) & 0x01;
            state->a = ral_temp;

            opbytes = 1;
            state->cycles += 4;
            break;
        }
    case 0x18:
        // 0x18	-
        opbytes = 1;
        break;
    case 0x19:
        // 0x19	DAD D	1	CY	HL = HL + DE
        {
            uint16_t hl_temp = (state->h << 8) + (state->l);
            uint16_t de_temp = (state->d << 8) + (state->e);

            uint32_t sum = hl_temp + de_temp;

            state->l = sum & 0xFF;
            state->h = sum >> 8 & 0xFF;

            // sum is 32 bits, in the even the 16th bit is set, it means there was a carry
            // mask sum with 0b00000000000000010000000000000000 (17th bit is 1, rest is 0) and check
            // if the mask results in 0x10000, which would mean that bit in on and that a carry happened
            state->cc.cy = (sum & 0x10000 == 0x10000) ? 1 : 0;

            state->cycles += 3;
            opbytes = 1;
            break;
        }
    case 0x1a:
        // 0x1a	LDAX D	1		A <- (DE)
        // Load whatever is in memory with address [whatever is contained in DE]
        state->a = state->memory[(state->d << 8) + (state->e)];
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0x1b:
        // 0x1b	DCX D	1		DE = DE-1
        {
            uint16_t de_temp = (state->d << 8) + (state->e);
            de_temp -= 1;

            state->e = de_temp & 0xFF;
            state->d = de_temp >> 8 & 0xFF;
            state->cycles += 5;
            opbytes = 1;
            break;
        }
    case 0x1c:
        // 0x1c	INR E	1	Z, S, P, AC	E <-E+1
        state->e += 1;

        SetFlags(state, state->c);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x1d:
        // 0x1d	DCR E	1	Z, S, P, AC	E <- E-1
        {
            state->e -= 1;

            SetFlags(state, state->e);

            opbytes = 1;
            state->cycles += 10;
            break;
        }
    case 0x1e:
        // 0x1e	MVI E,D8	2		E <- byte
        state->e = (state->memory[state->pc + 1]);
        // printf("Moved into E: %02x\n", state->e);
        opbytes = 2;
        state->cycles += 7;
        break;

    case 0x1f:
        // 0x1f	RAR	1	CY	A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
        {
            uint8_t rar_temp = (state->a >> 1) + ((state->cc.cy << 7) & 0x80);

            state->cc.cy = state->a & 0x01;
            state->a = rar_temp;

            opbytes = 1;
            state->cycles += 4;
            break;
        }
    case 0x20:
        // 0x20 -
        opbytes = 1;
        break;
    case 0x21:
        // 0x21	LXI H,D16	3		H <- byte 3, L <- byte 2
        state->h = (state->memory[state->pc + 2]);
        state->l = (state->memory[state->pc + 1]);
        // printf("Changed HL to %02x%02x\n", state->h, state->l);
        state->cycles += 10;
        opbytes = 3;
        break;
    case 0x22:
        // 0x22	SHLD adr	3		(adr) <-L; (adr+1)<-H
        {
            uint16_t adr = ((state->memory[state->pc + 2]) << 8) | (state->memory[state->pc + 1]);
            state->memory[adr] = state->l;
            state->memory[adr + 1] = state->h;

            opbytes = 3;
            state->cycles += 16;
            break;
        }
    case 0x23:
        // 0x23	INX H	1		HL <- HL + 1
        {
            uint16_t hl_temp = (state->h << 8) + (state->l);
            hl_temp += 1;

            state->l = hl_temp & 0xFF;
            state->h = hl_temp >> 8 & 0xFF;

            state->cycles += 5;
            opbytes = 1;
            break;
        }
    case 0x24:
        // 	0x24	INR H	1	Z, S, P, AC	H <- H+1
        state->h += 1;

        SetFlags(state, state->h);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x25:
        // 0x25	DCR H	1	Z, S, P, AC	H <- H-1
        state->h -= 1;

        SetFlags(state, state->h);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x26:
        // 0x26	MVI H,D8	2		H <- byte 2
        state->h = (state->memory[state->pc + 1]);
        // printf("Moved into H: %02x\n", state->h);
        opbytes = 2;
        state->cycles += 7;
        break;

    case 0x27:
        // DAA
        {
            uint8_t correction = 0;
            uint8_t old_acc = state->a; // Save the old accumulator value

            // Check lower nibble (bits 0-3)
            if ((state->a & 0x0F) > 0x9 || state->cc.ac)
            {
                // correction += 0x06;
                state->a += 0x06;
                state->cc.ac = 1; // Set auxiliary carry flag if adjustment is made
            }

            // Check upper nibble (bits 4-7)
            if (((state->a >> 4) & 0x0F) > 0x9 || state->cc.cy)
            {
                correction += 0x60;
                state->cc.cy = 1; // Set carry flag if adjustment is made
            }

            // Apply the correction to the accumulator
            SetFlags(state, state->a + correction);
            state->a = state->a + correction;

            // Set flags based on the new value in the accumulator

            // Maintain the carry flag if the result exceeds 8 bits
            if (state->a < old_acc)
            {
                state->cc.cy = 1; // Carry flag is set
            }
        }

    case 0x28:
        // 0x28 -
        opbytes = 1;
        break;
    case 0x29:
        // 0x29	DAD H	1	CY	HL = HL + HL
        {
            uint16_t hl_temp = (state->h << 8) + (state->l);
            uint16_t hl_temp2 = (state->h << 8) + (state->l);

            uint32_t sum = hl_temp + hl_temp2;

            state->l = sum & 0xFF;
            state->h = sum >> 8 & 0xFF;

            // sum is 32 bits, in the even the 16th bit is set, it means there was a carry
            // mask sum with 0b00000000000000010000000000000000 (17th bit is 1, rest is 0) and check
            // if the mask results in 0x10000, which would mean that bit in on and that a carry happened
            state->cc.cy = (sum & 0x10000 == 0x10000) ? 1 : 0;

            state->cycles += 3;
            opbytes = 1;
            break;
        }
    case 0x2a:
        // 0x2a	LHLD adr	3		L <- (adr); H<-(adr+1)
        {
            uint16_t adr = ((state->memory[state->pc + 2]) << 8) | (state->memory[state->pc + 1]);
            state->l = state->memory[adr];
            state->h = state->memory[adr + 1];

            opbytes = 3;
            state->cycles += 16;
            break;
        }
    case 0x2b:
    {
        // 0x2b	DCX H	1		HL = HL-1
        uint16_t hl_temp = (state->h << 8) + (state->l);
        hl_temp -= 1;

        state->l = hl_temp & 0xff;
        state->h = (hl_temp >> 8) & 0xff;
        state->cycles += 5;
        opbytes = 1;
        break;
    }
    case 0x2c:
        // 	0x2c	INR L	1	Z, S, P, AC	L <- L+1
        state->l += 1;

        SetFlags(state, state->l);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x2d:
        // 0x2d	DCR L	1	Z, S, P, AC	L <- L-1
        state->l -= 1;

        SetFlags(state, state->l);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x2e:
        // 0x2e	MVI L, D8	2		L <- byte 2
        state->l = (state->memory[state->pc + 1]);
        // printf("Moved into L: %02x\n", state->l);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x2f:
        // 0x2f	CMA	1		A <- !A
        {
            uint8_t comp_a = 0x00;
            int i;
            for (i = 8; i > 0; i--)
            {
                int inv_bit = !((state->a >> (i - 1)) & 0x01);
                comp_a = (inv_bit << (i - 1)) | comp_a;
            }
            // printf("A before complement: %02x", state->a);
            state->a = comp_a;
            // printf("A after complement: %02x", state->a);
            opbytes = 1;
            state->cycles += 1;
            break;
        }
    case 0x30:
        // 0x30	-
        opbytes = 1;
        break;
    case 0x31:
        // 0x31	LXI SP, D16	(3)		SP.hi <- byte 3, SP.lo <- byte 2
        state->sp = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        // printf("Changed SP to %02x\n", state->sp);
        state->cycles += 10;
        opbytes = 3;
        break;

    case 0x32:
    {
        // 0x32	STA adr	3		(adr) <- A
        uint16_t adr = ((state->memory[state->pc + 2]) << 8) | (state->memory[state->pc + 1]);
        state->memory[adr] = state->a;
        opbytes = 3;
        state->cycles += 4;
        break;
    }
    case 0x33:
        // 0x33	INX SP	1		SP = SP + 1

        state->sp += 1;

        opbytes = 1;
        state->cycles += 5;
        break;
    case 0x34:
    {
        // 	0x34	INR M	1	Z, S, P, AC	(HL) <- (HL)+1
        uint16_t hl = (state->h << 8) + (state->l);
        state->memory[hl] += 1;

        SetFlags(state, state->memory[hl]);

        opbytes = 1;
        state->cycles += 10;
        break;
    }
    case 0x35:
        // 0x35	DCR M	1	Z, S, P, AC	(HL) <- (HL)-1
        state->memory[(state->h << 8) | (state->l)] -= 1;

        SetFlags(state, state->memory[(state->h << 8) | (state->l)]);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x36:
    {
        // 0x36	MVI M,D8	2		(HL) <- byte 2
        uint16_t hl_temp = (state->h << 8) + (state->l);
        state->memory[hl_temp] = state->memory[state->pc + 1];

        state->cycles += 10;
        opbytes = 2;
        break;
    }
    case 0x37:
        // 0x37	STC	1	CY	CY = 1
        state->cc.cy = 1;
        opbytes = 1;
        state->cycles += 1;
        break;
    case 0x38:
        // 0x38	-
        opbytes = 1;
        break;
    case 0x39:
        // 0x39	DAD SP	1	CY	HL = HL + SP
        {
            uint16_t hl_temp = (state->h << 8) + (state->l);

            uint32_t sum = hl_temp + state->sp;

            state->l = sum & 0xFF;
            state->h = sum >> 8 & 0xFF;

            SetFlags(state, sum);
            state->cycles += 3;
            opbytes = 1;
            break;
        }
    case 0x3a:
    {
        // 0x3a	LDA adr	3		A <- (adr)
        uint16_t adr = ((state->memory[state->pc + 2]) << 8) | (state->memory[state->pc + 1]);
        state->a = (state->memory[adr]);

        opbytes = 3;
        state->cycles += 4;
        break;
    }
    case 0x3b:
        // 0x3b	DCX SP	1		SP = SP-1

        state->sp -= 1;

        opbytes = 1;
        state->cycles += 5;
        break;
    case 0x3c:
    {
        // 	0x3c	INR A	1	Z, S, P, AC	A <- A+1
        state->a += 1;

        SetFlags(state, state->a);

        opbytes = 1;
        state->cycles += 10;
        break;
    }
    case 0x3d:
        // 0x3d	DCR A	1	Z, S, P, AC	A <- A-1
        state->a -= 1;

        SetFlags(state, state->a);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x3e:
        // 0x2e	MVI L, D8	2		L <- byte 2
        state->a = (state->memory[state->pc + 1]);
        // printf("Moved into A: %02x\n", state->a);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x3f:
        // 0x3f	CMC	1	CY	CY=!CY
        state->cc.cy = !(state->cc.cy);
        opbytes = 1;
        state->cycles += 1;
        break;
    case 0x40:
        // 0x40  MOV B,B  1       B <- B
        state->b = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x41:
        // 0x41  MOV B,C  1       B <- C
        state->b = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x42:
        // 0x42  MOV B,D  1       B <- D
        state->b = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x43:
        // 0x43  MOV B,E  1       B <- E
        state->b = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x44:
        // 0x44  MOV B,H  1       B <- H
        state->b = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x45:
        // 0x45  MOV B,L  1       B <- L
        state->b = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x46:
        // 0x46  MOV B,M  1       B <- (HL)
        state->b = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x47:
        // 0x47  MOV B,A  1       B <- A
        state->b = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x48:
        // 0x48  MOV C,B  1       C <- B
        state->c = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x49:
        // 0x49  MOV C,C  1       C <- C
        state->c = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x4a:
        // 0x4a  MOV C,D  1       C <- D
        state->c = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x4b:
        // 0x4b  MOV C,E  1       C <- E
        state->c = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x4c:
        // 0x4c  MOV C,H  1       C <- H
        state->c = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x4d:
        // 0x4d  MOV C,L  1       C <- L
        state->c = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x4e:
        // 0x4e  MOV C,M  1       C <- (HL)
        state->c = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x4f:
        // 0x4f  MOV C,A  1       C <- A
        state->c = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x50:
        // 0x50  MOV D,B  1       D <- B
        state->d = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x51:
        // 0x51  MOV D,C  1       D <- C
        state->d = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x52:
        // 0x52  MOV D,D  1       D <- D
        state->d = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x53:
        // 0x53  MOV D,E  1       D <- E
        state->d = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x54:
        // 0x54  MOV D,H  1       D <- H
        state->d = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x55:
        // 0x55  MOV D,L  1       D <- L
        state->d = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x56:
        // 0x56  MOV D,M  1       D <- (HL)
        state->d = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x57:
        // 0x57  MOV D,A  1       D <- A
        state->d = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x58:
        // 0x58  MOV E,B  1       E <- B
        state->e = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x59:
        // 0x59  MOV E,C  1       E <- C
        state->e = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x5a:
        // 0x5a  MOV E,D  1       E <- D
        state->e = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x5b:
        // 0x5b  MOV E,E  1       E <- E
        state->e = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x5c:
        // 0x5c  MOV E,H  1       E <- H
        state->e = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x5d:
        // 0x5d  MOV E,L  1       E <- L
        state->e = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x5e:
        // 0x5e  MOV E,M  1       E <- (HL)
        state->e = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x5f:
        // 0x5f  MOV E,A  1       E <- A
        state->e = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x60:
        // 0x60  MOV H,B  1       H <- B
        state->h = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x61:
        // 0x61  MOV H,C  1       H <- C
        state->h = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x62:
        // 0x62  MOV H,D  1       H <- D
        state->h = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x63:
        // 0x63  MOV H,E  1       H <- E
        state->h = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x64:
        // 0x64  MOV H,H  1       H <- H
        state->h = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x65:
        // 0x65  MOV H,L  1       H <- L
        state->h = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x66:
        // 0x66  MOV H,M  1       H <- (HL)
        state->h = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x67:
        // 0x67  MOV H,A  1       H <- A
        state->h = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x68:
        // 0x68  MOV L,B  1       L <- B
        state->l = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x69:
        // 0x69  MOV L,C  1       L <- C
        state->l = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x6a:
        // 0x6a  MOV L,D  1       L <- D
        state->l = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x6b:
        // 0x6b  MOV L,E  1       L <- E
        state->l = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x6c:
        // 0x6c  MOV L,H  1       L <- H
        state->l = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x6d:
        // 0x6d  MOV L,L  1       L <- L
        state->l = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x6e:
        // 0x6e  MOV L,M  1       L <- (HL)
        state->l = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x6f:
        // 0x6f  MOV L,A  1       L <- A
        state->l = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x70:
        // 0x70  MOV M,B  1       (HL) <- B
        state->memory[(state->h << 8) | state->l] = state->b;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x71:
        // 0x71  MOV M,C  1       (HL) <- C
        state->memory[(state->h << 8) | state->l] = state->c;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x72:
        // 0x72  MOV M,D  1       (HL) <- D
        state->memory[(state->h << 8) | state->l] = state->d;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x73:
        // 0x73  MOV M,E  1       (HL) <- E
        state->memory[(state->h << 8) | state->l] = state->e;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x74:
        // 0x74  MOV M,H  1       (HL) <- H
        state->memory[(state->h << 8) | state->l] = state->h;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x75:
        // 0x75  MOV M,L  1       (HL) <- L
        state->memory[(state->h << 8) | state->l] = state->l;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x76:
        // 0x76  HLT  1       special
        // Halt execution (special handling might be needed)
        state->cycles += 4;
        opbytes = 1;
        break;

    case 0x77:
        // 0x77  MOV M,A  1       (HL) <- A
        state->memory[(state->h << 8) | state->l] = state->a;
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x78:
        // 0x78  MOV A,B  1       A <- B
        state->a = state->b;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x79:
        // 0x79  MOV A,C  1       A <- C
        state->a = state->c;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x7a:
        // 0x7a  MOV A,D  1       A <- D
        state->a = state->d;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x7b:
        // 0x7b  MOV A,E  1       A <- E
        state->a = state->e;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x7c:
        // 0x7c  MOV A,H  1       A <- H
        state->a = state->h;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x7d:
        // 0x7d  MOV A,L  1       A <- L
        state->a = state->l;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x7e:
        // 0x7e  MOV A,M  1       A <- (HL)
        state->a = state->memory[(state->h << 8) | state->l];
        state->cycles += 7;
        opbytes = 1;
        break;

    case 0x7f:
        // 0x7f  MOV A,A  1       A <- A
        state->a = state->a;
        state->cycles += 5;
        opbytes = 1;
        break;

    case 0x80:
        // 0x80	ADD B	1	Z, S, P, CY, AC	A <- A + B
        {
            uint16_t res = state->a + state->b;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x81:
        // 0x81	ADD C	1	Z, S, P, CY, AC	A <- A + C
        {
            uint16_t res = state->a + state->c;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x82:
        // 0x82	ADD D	1	Z, S, P, CY, AC	A <- A + D
        SetFlags(state, state->a + state->d);
        state->a = state->a + state->d;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x83:
        // 0x83	ADD E	1	Z, S, P, CY, AC	A <- A + E
        SetFlags(state, state->a + state->e);
        state->a = state->a + state->e;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x84:
        // 0x84	ADD H	1	Z, S, P, CY, AC	A <- A + H
        SetFlags(state, state->a + state->h);
        state->a = state->a + state->h;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x85:
        // 0x85	ADD L	1	Z, S, P, CY, AC	A <- A + L
        SetFlags(state, state->a + state->l);
        state->a = state->a + state->l;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x86:
        // 0x86	ADD M	1	Z, S, P, CY, AC	A <- A + (HL)
        {
            uint16_t hl = (state->h << 8) + (state->l);
            SetFlags(state, state->a + state->memory[hl]);
            state->a = state->a + state->memory[hl];
            state->cycles += 7;
            opbytes = 1;
            break;
        }
    case 0x87:
        // 0x87	ADD A	1	Z, S, P, CY, AC	A <- A + A
        SetFlags(state, state->a + state->a);
        state->a = state->a + state->a;
        state->cycles += 4;
        opbytes = 1;
        break;

    case 0x88:
        // 0x88	ADC B	1	Z, S, P, CY, AC	A <- A + B + CY
        {
            uint16_t res = state->a + state->b + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x89:
        // 0x89	ADC C	1	Z, S, P, CY, AC	A <- A + C + CY
        {
            uint16_t res = state->a + state->c + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8a:
        // 0x8a	ADC D	1	Z, S, P, CY, AC	A <- A + D + CY
        {
            uint8_t a_temp = state->a;

            uint16_t res = state->a + state->d + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8b:
        // 0x8b	ADC E	1	Z, S, P, CY, AC	A <- A + E + CY
        {
            uint16_t res = state->a + state->e + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8c:
        // 0x8c	ADC H	1	Z, S, P, CY, AC	A <- A + H + CY
        {
            uint16_t res = state->a + state->h + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8d:
        // 0x8d	ADC L	1	Z, S, P, CY, AC	A <- A + L + CY
        {
            uint16_t res = state->a + state->l + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8e:
        // 0x8e	ADC M	1	Z, S, P, CY, AC	A <- A + (HL) + CY
        {
            uint16_t res = state->a + state->memory[(state->h << 8) + (state->l)] + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x8f:
        // 0x8f	ADC A	1	Z, S, P, CY, AC	A <- A + A + CY
        {
            uint16_t res = state->a + state->a + state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }

        // SUBs
    case 0x90:
        // 0x90	SUB B	1	Z, S, P, CY, AC	A <- A - B
        SetFlags(state, state->a - state->b);
        state->a = state->a - state->b;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x91:
        // 0x91	SUB C	1	Z, S, P, CY, AC	A <- A - C
        SetFlags(state, state->a - state->c);
        state->a = state->a - state->c;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x92:
        // 0x92	SUB D	1	Z, S, P, CY, AC	A <- A - D
        SetFlags(state, state->a - state->d);
        state->a = state->a - state->d;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x93:
        // 0x93	SUB E	1	Z, S, P, CY, AC	A <- A - E
        SetFlags(state, state->a - state->e);
        state->a = state->a - state->e;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x94:
        // 0x94	SUB H	1	Z, S, P, CY, AC	A <- A - H
        SetFlags(state, state->a - state->h);
        state->a = state->a - state->h;
        state->cycles += 4;
        opbytes = 1;
        break;
    case 0x95:
        // 0x95	SUB L	1	Z, S, P, CY, AC	A <- A - L
        SetFlags(state, state->a - state->l);
        state->a = state->a - state->l;
        state->cycles += 4;
        opbytes = 1;
        break;

    case 0x96:
        // 0x96	SUB M	1	Z, S, P, CY, AC	A <- A - (HL)
        SetFlags(state, state->a - state->memory[(state->h << 8) + (state->l)]);
        state->a = state->a - state->memory[(state->h << 8) + (state->l)];
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0x97:
        // 0x97	SUB A	1	Z, S, P, CY, AC	A <- A - A
        SetFlags(state, state->a - state->a);
        state->a = state->a - state->a;
        state->cycles += 4;
        opbytes = 1;
        break;

    case 0x98:
        // 0x98	SBB B	1	Z, S, P, CY, AC	A <- A - B - CY
        {
            uint16_t res = state->a - state->b - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x99:
        // 0x99	SBB C	1	Z, S, P, CY, AC	A <- A - C - CY
        {
            uint16_t res = state->a - state->c - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9a:
        // 0x9a	SBB D	1	Z, S, P, CY, AC	A <- A - D - CY
        {
            uint16_t res = state->a - state->d - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9b:
        // 0x9b	SBB E	1	Z, S, P, CY, AC	A <- A - E - CY
        {
            uint16_t res = state->a - state->e - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9c:
        // 0x9c	SBB H	1	Z, S, P, CY, AC	A <- A - H - CY
        {
            uint16_t res = state->a - state->h - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9d:
        // 0x9d	SBB L	1	Z, S, P, CY, AC	A <- A - L - CY
        {
            uint16_t res = state->a - state->l - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9e:
        // 0x9e	SBB M	1	Z, S, P, CY, AC	A <- A - (HL) - CY
        {
            uint16_t res = state->a - state->memory[(state->h << 8) + (state->l)] - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
    case 0x9f:
        // 0x9f	SBB A	1	Z, S, P, CY, AC	A <- A - A - CY
        {
            uint16_t res = state->a - state->a - state->cc.cy;
            state->a = res;
            SetFlags(state, res);
            state->cycles += 4;
            opbytes = 1;
            break;
        }
        // ANDs
    case 0xa0:
        // 0xa0	ANA B	1	Z, S, P, CY, AC	A <- A & B
        state->a = state->a & state->b;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa1:
        // 0xa1	ANA C	1	Z, S, P, CY, AC	A <- A & C
        state->a = state->a & state->c;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa2:
        // 0xa2	ANA D	1	Z, S, P, CY, AC	A <- A & D
        state->a = state->a & state->d;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa3:
        // 0xa3	ANA E	1	Z, S, P, CY, AC	A <- A & E
        state->a = state->a & state->e;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa4:
        // 0xa4	ANA H	1	Z, S, P, CY, AC	A <- A & H
        state->a = state->a & state->h;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa5:
        // 0xa5	ANA L	1	Z, S, P, CY, AC	A <- A & L
        state->a = state->a & state->l;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa6:
        // 0xa6	ANA M	1	Z, S, P, CY, AC	A <- A & (HL)
        state->a = state->a & state->memory[(state->h << 8) + (state->l)];
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xa7:
        // 0xa7	ANA A	1	Z, S, P, CY, AC	A <- A & A
        state->a = state->a & state->a;
        SetFlags(state, state->a);

        // clear CY
        state->cc.cy = 0;
        state->cycles += 7;
        opbytes = 1;
        break;

        // XORs
    case 0xa8:
        // 0xa8	XRA B	1	Z, S, P, CY, AC	A <- A ^ B
        state->a = state->a ^ state->b;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa9:
        // 0xa9	XRA C	1	Z, S, P, CY, AC	A <- A ^ C
        state->a = state->a ^ state->c;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xaa:
        // 0xaa	XRA D	1	Z, S, P, CY, AC	A <- A ^ D
        state->a = state->a ^ state->d;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xab:
        // 0xab	XRA E	1	Z, S, P, CY, AC	A <- A ^ E
        state->a = state->a ^ state->e;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xac:
        // 0xac	XRA H	1	Z, S, P, CY, AC	A <- A ^ H
        state->a = state->a ^ state->h;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xad:
        // 0xad	XRA L	1	Z, S, P, CY, AC	A <- A ^ L
        state->a = state->a ^ state->l;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 7;
        opbytes = 1;
        break;
    case 0xae:
        // 0xae	XRA M	1	Z, S, P, CY, AC	A <- A ^ (HL)
        state->a = state->a ^ state->memory[(state->h << 8) + (state->l)];
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 2;
        opbytes = 1;
        break;
    case 0xaf:
        // 0xaf	XRA A	1	Z, S, P, CY, AC	A <- A ^ A
        state->a = state->a ^ state->a;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb0:
        // 0xb0	ORA B	1	Z, S, P, CY, AC	A <- A | B
        state->a = state->a | state->b;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb1:
        // 0xb1	ORA C	1	Z, S, P, CY, AC	A <- A | C
        state->a = state->a | state->c;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb2:
        // 0xb2	ORA D	1	Z, S, P, CY, AC	A <- A | D
        state->a = state->a | state->d;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb3:
        // 0xb3	ORA E	1	Z, S, P, CY, AC	A <- A | E
        state->a = state->a | state->e;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb4:
        // 0xb4	ORA H	1	Z, S, P, CY, AC	A <- A | H
        state->a = state->a | state->h;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb5:
        // 0xb5	ORA L	1	Z, S, P, CY, AC	A <- A | L
        state->a = state->a | state->l;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb6:
        // 0xb6	ORA M	1	Z, S, P, CY, AC	A <- A | (HL)
        state->a = state->a | state->memory[(state->h << 8) + (state->l)];
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 2;
        opbytes = 1;
        break;

    case 0xb7:
        // 0xb7	ORA A	1	Z, S, P, CY, AC	A <- A | A
        state->a = state->a | state->a;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
        opbytes = 1;
        break;

    case 0xb8:
        // 0xb8	CMP B	1	Z, S, P, CY, AC	A - B
        SetFlags(state, state->a - state->b);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xb9:
        // 0xb9	CMP C	1	Z, S, P, CY, AC	A - C
        SetFlags(state, state->a - state->c);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xba:
        // 0xba	CMP D	1	Z, S, P, CY, AC	A - D
        SetFlags(state, state->a - state->d);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xbb:
        // 0xbb	CMP E	1	Z, S, P, CY, AC	A - E
        SetFlags(state, state->a - state->e);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xbc:
        // 0xbc	CMP H	1	Z, S, P, CY, AC	A - H
        SetFlags(state, state->a - state->h);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xbd:
        // 0xbd	CMP L	1	Z, S, P, CY, AC	A - L
        SetFlags(state, state->a - state->l);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xbe:
        // 0xbe	CMP M	1	Z, S, P, CY, AC	A - (HL)
        SetFlags(state, state->a - state->memory[(state->h << 8) | (state->l)]);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xbf:
        // 0xbf	CMP A	1	Z, S, P, CY, AC	A - A
        SetFlags(state, state->a - state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xc0:
        // 0xc0	RNZ	1		if NZ, RET
        if (state->cc.z == 0)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xc1:
        // 0xc1	POP B	1		C <- (sp); B <- (sp+1); sp <- sp+2
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;
    case 0xc2:
        // 0xc2	JNZ adr	3		if NZ, PC <- adr
        if (state->cc.z == 0)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on condition NZ: %02x\n", address);
            state->pc = address - 1;

        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 10;
        break;
    case 0xc3:
        // 0xc3 JMP adr	(3)		PC <= adr
        address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        // printf("Jumping to address %02x\n", address);
        state->pc = address;
        opbytes = 0;
        state->cycles += 10;
        break;
    case 0xc4:
        // 0xc4	CNZ adr	3		if NZ, CALL adr
        if (state->cc.z == 0)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nWhat was done\n\n");
            // printf("New SP: %02x\n", state->sp);
            // printf("New PC: %02x\n", state->pc);
            // printf("Memory: %04x\n", (state->memory[(state->sp) + 1] << 8) | state->memory[(state->sp)]);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;

    case 0xc5:
        // 0xc5	PUSH B	1		(sp-2)<-C; (sp-1)<-B; sp <- sp - 2
        // printf("Executuing 0xc5 PUSH BC");
        state->memory[state->sp - 2] = state->c;
        state->memory[state->sp - 1] = state->b;
        state->sp -= 2;

        opbytes = 1;
        state->cycles += 3;
        break;
    case 0xc6:
        // 0xc6	ADI D8	2	Z, S, P, CY, AC	A <- A + byte
        {
            uint16_t res = state->a;
            res = res + state->memory[state->pc + 1];

            // printf("adi res: %04x\n", res);
            // printf("CY: %d\n", (res > 0xFF) ? 1 : 0);

            SetFlags(state, res);

            state->a = res & 0xff;

            opbytes = 2;
            state->cycles += 2;
            break;
        }
    case 0xc8:
        // 0xc8	RZ	1		if Z, RET
        if (state->cc.z == 1)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call

            // printf("Retuning on zero... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;

            // opbytes = 3;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 10;
        break;
    case 0xc9:
        // 0xc9	RET	1		PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2
        state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 2; // continue at the address right after the conditional call
        // printf("Retuning... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
        state->sp += 2;

        opbytes = 1;
        state->cycles += 10;
        break;

    case 0xca:
        // 0xca	JZ adr	3		if Z, PC <- adr
        if (state->cc.z == 1)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on condition Z: %02x\n", address);
            state->pc = address - 1;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 10;
        break;

    case 0xcb:
        // 0xcb	-
        opbytes = 1;
        break;
    case 0xcc:
    {
        // 0xcc	CZ adr	3		if Z, CALL adr
        if (state->cc.z == 1)
        {
            // printf("Calling... SP-2=%02x, SP-1=%02x, storing %02x\n", (state->sp) - 2, (state->sp) - 1, state->pc);
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Putting address %02x in PC", address);
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);

            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;

            opbytes = 0;
        }
        else
        {
            opbytes = 3;
        }
        state->cycles += 17;

        break;
    }
    case 0xcd:
// 0xcd	CALL adr	3		(SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr
#ifdef FOR_CPUDIAG
        if (0x105 == ((opcode[2] << 8) | opcode[1]))
        {
            if (state->c == 9)
            {
                uint16_t offset = (state->d << 8) | (state->e);
                char *str = &state->memory[offset + 3]; // skip the prefix bytes
                while (*str != '$')
                    printf("%c", *str++);
                printf("\n");
                exit(0);
            }
            else if (state->c == 2)
            {
                // saw this in the inspected code, never saw it called
                printf("print char routine called\n");
            }
        }
        else if (0 == ((opcode[2] << 8) | opcode[1]))
        {
            exit(0);
        }
        else
#endif // using the content of SP as reference address, load PC (2 bytes) into the 2 memory addresses that are before the reference (SP)

            // printf("Calling... SP-2=%02x, SP-1=%02x, storing %02x\n", (state->sp) - 2, (state->sp) - 1, state->pc);
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
        state->memory[(state->sp) - 1] = state->pc >> 8;       // PC.hi
        state->sp = (state->sp) - 2;

        address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        // printf("Putting address %02x in PC", address);
        state->pc = address;

        // printf("\n\nState after call\n\n");
        // printf("SP: %02x\n", state->sp);
        // printf("PC: %02x\n", state->pc);
        stack[stack_size] = ((state->memory[(state->sp) + 1]) >> 8) | (state->memory[(state->sp)]);
        stack_size++;
        opbytes = 0;
        state->cycles += 17;
        break;
    case 0xce:
        // 0xce	ACI D8	2	Z, S, P, CY, AC	A <- A + data + CY
        {
            uint16_t res = state->a + state->memory[state->pc + 1] + state->cc.cy;
            SetFlags(state, res);
            state->a = res & 0xff;
            // printf("res after ACI: %04x\n", res);
            // printf("res && 0xff after ACI: %04x\n", res & 0xff);
            state->cycles += 2;
            opbytes = 2;
            break;
        }
    case 0xd0:
        // 0xd0	RNC	1		if NCY, RET
        if (state->cc.cy == 0)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xd1:
        // 0xd1	POP D	1		E <- (sp); D <- (sp+1); sp <- sp+2
        state->e = state->memory[state->sp];
        state->d = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;

        break;
    case 0xd2:
        // 0xd2	JNC adr	3		if NCY, PC<-adr
        if (state->cc.cy == 0)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on condition NZ: %02x\n", address);
            state->pc = address - 1;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 10;
        break;
    case 0xd3:
        // state->bus[state->pc + 1] = state->a;
        opbytes = 2;
        state->cycles += 3;
        break;
    case 0xd4:
        // 0xd4	CNC adr	3		if NCY, CALL adr
        if (state->cc.cy == 0)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xd5:
        // 0xd5	PUSH D	1		(sp-2)<-E; (sp-1)<-D; sp <- sp - 2
        state->memory[state->sp - 2] = state->e;
        state->memory[state->sp - 1] = state->d;
        state->sp -= 2;

        opbytes = 1;
        state->cycles += 3;
        break;
    case 0xd6:
        // 0xd6	SUI D8	2	Z, S, P, CY, AC	A <- A - data
        {
            uint16_t res = state->a - state->memory[state->pc + 1];
            state->a = res & 0xff;
            SetFlags(state, res);
            state->cycles += 7;
            opbytes = 2;
            break;
        }
    case 0xd8:
        // 0xd8	RC	1		if CY, RET
        if (state->cc.cy == 1)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xd9:
        // 0xd9	-
        opbytes = 1;
        break;
    case 0xda:
    {
        // 0xda	JC adr	3		if CY, PC<-adr
        if (state->cc.cy == 1)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on Minus %02x\n", address);
            state->pc = address;
            opbytes = 0;
        }
        else
        {
            opbytes = 3;
        }
        state->cycles += 10;
        break;
    }
    case 0xdc:
        // 0xdc	CC adr	3		if CY, CALL adr
        if (state->cc.cy == 1)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xdd:
        // 0xdd	-
        opbytes = 1;
        break;
    case 0xde:
        // 0xde	SBI D8	2	Z, S, P, CY, AC	A <- A - data - CY
        {
            uint16_t res = state->a - state->memory[state->pc + 1] - state->cc.cy;
            SetFlags(state, res);
            state->a = res & 0xff;
            state->cycles += 2;
            opbytes = 2;
            break;
        }
    case 0xe0:
        // 0xe0	RPO	1		if PO, RET
        if (state->cc.p == 0)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xe1:
        // 0xe1	POP H	1		L <- (sp); H <- (sp+1); sp <- sp+2
        state->l = state->memory[state->sp];
        state->h = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;
    case 0xe2:
        // 0xe2	JPO adr	3		if PO, PC <- adr
        if (state->cc.p == 0)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on condition NZ: %02x\n", address);
            state->pc = address - 1;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 10;
        break;
    case 0xe3:
        // 0xe3	XTHL	1		L <-> (SP); H <-> (SP+1)
        {
            uint8_t h_temp = state->h;
            uint8_t l_temp = state->l;

            state->h = state->memory[state->sp + 1];
            state->l = state->memory[state->sp];

            state->memory[state->sp] = l_temp;
            state->memory[state->sp + 1] = h_temp;

            state->cycles += 18;
            opbytes = 1;
            break;
        }

    case 0xe4:
        // 0xe4	CPO adr	3		if PO, CALL adr
        if (state->cc.p == 0)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xe5:
        // 0xe5	PUSH H	1		(sp-2)<-L; (sp-1)<-H; sp <- sp - 2
        state->memory[state->sp - 2] = state->l;
        state->memory[state->sp - 1] = state->h;
        state->sp -= 2;

        opbytes = 1;
        state->cycles += 3;
        break;
    case 0xe6:
        // 0xe6	ANI D8	2	Z, S, P, CY, AC	A <- A & data
        state->a = state->a & state->memory[state->pc + 1];
        SetFlags(state, state->a);
        state->cc.cy = 0;
        state->cc.ac = 0;
        opbytes = 2;
        state->cycles += 2;
        break;
    case 0xe8:
        // 0xe8	RPE	1		if PE, RET
        if (state->cc.p == 1)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xe9:
        // 0xe9	PCHL	1		PC.hi <- H; PC.lo <- L

        state->pc = (state->h << 8) | state->l;
        opbytes = 0;
        state->cycles += 5;
        break;
    case 0xea:
    {
        // 0xea	JPE adr	3		if PE, PC <- adr
        address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        // printf("Jumping to address on parity even %02x\n", address);
        if (state->cc.p == 1)
        {
            state->pc = address;
            opbytes = 0;
        }
        else
        {
            opbytes = 3;
        }
        state->cycles += 10;
        break;
    }
    case 0xeb:
        // 0xeb	XCHG	1		H <-> D; L <-> E
        {
            uint8_t h_temp = state->h;
            uint8_t l_temp = state->l;

            state->h = state->d;
            state->l = state->e;
            state->d = h_temp;
            state->e = l_temp;

            state->cycles += 1;
            opbytes = 1;
            break;
        }
    case 0xec:
        // 0xec	CPE adr	3		if PE, CALL adr
        if (state->cc.p == 1)
        {
            // printf("Calling on PE... SP-2=%02x, SP-1=%02x, storing %02x\n", (state->sp) - 2, (state->sp) - 1, state->pc);
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xed:
        // 0xed	-
        opbytes = 1;
        break;

    case 0xee:
        // 0xee	XRI D8	2	Z, S, P, CY, AC	A <- A ^ data
        state->a = state->a ^ state->memory[state->pc + 1];
        SetFlags(state, state->a);
        state->cc.cy = 0;
        state->cc.ac = 0;
        opbytes = 2;
        state->cycles += 2;
        break;

    case 0xf0:
        // 0xf0	RP	1		if P (cc.s = 0), RET
        if (state->cc.s == 0)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xf1:
        // 0xf1	POP PSW	1		flags <- (sp); A <- (sp+1); sp <- sp+2
        {
            uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (0 << 5) + (state->cc.ac << 4) + (0 << 3) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);

            state->cc.s = (state->memory[state->sp] >> 7) & 0x01;
            state->cc.z = (state->memory[state->sp] >> 6) & 0x01;
            state->cc.ac = (state->memory[state->sp] >> 4) & 0x01;
            state->cc.p = (state->memory[state->sp] >> 2) & 0x01;
            state->cc.cy = (state->memory[state->sp] & 0x01);

            // state->memory[state->sp - 2] = flags;
            state->a = state->memory[state->sp + 1];
            state->sp += 2;

            opbytes = 1;
            state->cycles += 3;
            break;
        }
    case 0xf2:
    {
        // 0xf2	JP adr	3		if P=1 PC <- adr
        address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        // printf("Jumping to address on parity even %02x\n", address);
        if (state->cc.s == 0)
        {
            state->pc = address;
            opbytes = 0;
        }
        else
        {
            opbytes = 3;
        }
        state->cycles += 10;
        break;
    }
    case 0xf3:
        // 0xf3	DI	1		special
        {
            state->int_enabled = 0x00;
            opbytes = 1;
            state->cycles += 1;
            break;
        }
    case 0xf4:
        // 0xf4	CP adr	3		if P, PC <- adr
        if (state->cc.s == 0)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xf5:
        // 0xf5	PUSH PSW	1		(sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
        {
            // printf("Executing 0xf5 PUSH PSW");
            uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (0 << 5) + (state->cc.ac << 4) + (0 << 3) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);
            state->memory[state->sp - 2] = flags;
            state->memory[state->sp - 1] = state->a;
            state->sp -= 2;

            opbytes = 1;
            state->cycles += 3;
            break;
        }
    case 0xf6:
        // 0xf6	ORI D8	2	Z, S, P, CY, AC	A <- A | data
        state->a = state->a | state->memory[state->pc + 1];
        SetFlags(state, state->a);
        state->cc.cy = 0;
        state->cc.ac = 0;
        opbytes = 2;
        state->cycles += 2;
        break;
    case 0xf8:
        // 0xf8	RM	1		if M, RET
        if (state->cc.s == 1)
        {
            state->pc = ((state->memory[state->sp + 1] << 8) + (state->memory[state->sp])) + 3; // continue at the address right after the conditional call
            // printf("Retuning on P... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
            state->sp += 2;
            opbytes = 0;
        }
        else
        {
            opbytes = 1;
        }
        state->cycles += 3;
        break;
    case 0xf9:
        // 0xf9	SPHL	1		SP=HL
        {
            uint16_t hl = (state->h << 8) + (state->l);
            state->sp = hl;

            opbytes = 1;
            state->cycles += 1;
            break;
        }
    case 0xfa:
    {
        // 0xfa	JM adr	3		if M, PC <- adr
        if (state->cc.s == 1)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            // printf("Jumping to address on Minus %02x\n", address);
            state->pc = address;
            opbytes = 0;
        }
        else
        {
            opbytes = 3;
        }
        state->cycles += 10;
        break;
    }
    case 0xfb:
        // 0xfb	EI	1		special
        {
            state->int_enabled = 0x01;
            opbytes = 1;
            state->cycles += 1;

            break;
        }
    case 0xfc:
        // 0xfc	CM adr	3		if M, CALL adr
        if (state->cc.s == 1)
        {
            state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
            state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
            state->sp = (state->sp) - 2;

            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            state->pc = address;

            // printf("\n\nState after call\n\n");
            // printf("SP: %02x\n", state->sp);
            // printf("PC: %02x\n", state->pc);
            opbytes = 0;
            stack[stack_size] = ((state->memory[(state->sp) + 1]) << 8) | (state->memory[(state->sp)]);
            stack_size++;
        }
        else
        {
            opbytes = 3;
        }

        state->cycles += 17;
        break;
    case 0xfd:
        // 0xfd	-
        opbytes = 1;
        break;
    case 0xfe:
    {
        // 0xfe	CPI D8	2	Z, S, P, CY, AC	A - data
        uint16_t comp = state->a - state->memory[state->pc + 1];
        // printf("CPI: %02x - %02x = %02x\n", state->a, state->memory[state->pc + 1], comp);

        SetFlags(state, comp);

        state->cycles += 7;
        opbytes = 2;
        break;
    }

    default:
        UnimplementedInstruction(state);
    }

    return opbytes;
}

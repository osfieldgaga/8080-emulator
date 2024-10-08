#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    state->a = 0;
    state->b = 0;
    state->c = 0;
    state->d = 0;
    state->e = 0;
    state->h = 0;
    state->l = 0;
    state->sp = 0;
    state->pc = 0;
}

void InitializeMemory(State8080 *state)
{
    // allocate memory for 16KB (8KB ROM + 8KB RAM)
    state->memory = (uint8_t *)malloc(0x4000);
}

void ShowState(State8080 *state)
{
    uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (state->cc.ac << 4) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);

    printf("\nSP: %02x\n", state->sp);
    printf("PC: %02x\n", state->pc);
    printf("A: %02x\n", state->a);
    // printf("F: %02x\n", flags);
    printf("B: %02x\n", state->b);
    printf("C: %02x\n", state->c);
    printf("D: %02x\n", state->d);
    printf("E: %02x\n", state->e);
    printf("H: %02x\n", state->h);
    printf("L: %02x\n\n", state->l);
    printf("Cycles: %d\n\n", state->cycles);
    printf("CALL/RET content: %02x%02x\n", state->memory[state->sp + 1], state->memory[state->sp]);

    printf("S  Z   AC   P   C\n");
    printf("%x  %x   %x    %x   %x\n\n", state->cc.s, state->cc.z, state->cc.ac, state->cc.p, state->cc.cy);
}

void SetFlags(State8080 *state, uint16_t ops_result)
{
    state->cc.z = (ops_result == 0) ? 1 : 0;
    state->cc.s = (ops_result >> 7 & 0x01 == 1) ? 1 : 0;
    state->cc.p = (ops_result % 2 == 0) ? 1 : 0;

    state->cc.ac = ((((ops_result & 0xF) - 1) & 0x10) == 0x10) ? 1 : 0;

    // ops_result is assumed 32 bits, in the even the 16th bit is set, it means there was a carry.
    // mask sum with 0b00000000000000010000000000000000 (17th bit is 1, rest is 0) and check
    // if the mask results in 0x10000, which would mean that bit in on and that a carry happened
    state->cc.cy = (ops_result & 0x10000 == 0x10000) ? 1 : 0;
}

void UnimplementedInstruction(State8080 *state)
{
    uint8_t *opcode = &state->memory[state->pc];
    printf("Instruction %02x not implemented yet.\n", *opcode);

    printf("\n\nState on unimplemented\n");
    ShowState(state);

    exit(1);
}

int Emulate8080(State8080 *state)
{
    int opbytes = 1;
    uint8_t *opcode = &state->memory[state->pc];
    uint16_t address;

    printf("Executing opcode: %02x, PC is %02x\n", *opcode, state->pc);

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
        printf("Changed BC to %02x%02x\n", state->b, state->c);
        opbytes = 3;
        state->cycles += 10;
        break;
    case 0x05:
        // 0x05	DCR B	1	Z, S, P, AC	B <- B-1
        state->b -= 1;

        SetFlags(state, state->b);

        printf("%02x", (state->b & 0xF));

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x06:
        // 0x06	MVI B, D8	2		B <- byte 2
        state->b = (state->memory[state->pc + 1]);
        printf("Moved into B: %02x\n", state->b);
        opbytes = 2;
        state->cycles += 7;
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
            state->cc.cy = (sum & 0x10000 == 0x10000) ? 1 : 0;

            state->cycles += 3;
            opbytes = 1;
            break;
        }
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
        printf("Moved into C: %02x\n", state->c);
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
    case 0x11:
        // 0x11	LXI D,D16	3		D <- byte 3, E <- byte 2
        state->d = (state->memory[state->pc + 2]);
        state->e = (state->memory[state->pc + 1]);
        printf("Changed DE to %02x%02x\n", state->d, state->e);
        opbytes = 3;
        state->cycles += 10;
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
    case 0x15:
        // 0x15	DCR D	1	Z, S, P, AC	D <- D-1
        state->d -= 1;

        SetFlags(state, state->d);

        opbytes = 1;
        state->cycles += 10;
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
    case 0x1d:
        // 0x1d	DCR E	1	Z, S, P, AC	E <- E-1
        state->e -= 1;

        SetFlags(state, state->e);

        opbytes = 1;
        state->cycles += 10;
        break;
    case 0x16:
        // 0x16	MVI D, D8	2		D <- byte 2
        state->d = (state->memory[state->pc + 1]);
        printf("Moved into D: %02x\n", state->d);
        opbytes = 2;
        state->cycles += 7;
        break;

    case 0x1e:
        // 0x1e	MVI E,D8	2		E <- byte
        state->e = (state->memory[state->pc + 1]);
        printf("Moved into E: %02x\n", state->e);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x21:
        // 0x21	LXI H,D16	3		H <- byte 3, L <- byte 2
        state->h = (state->memory[state->pc + 2]);
        state->l = (state->memory[state->pc + 1]);
        printf("Changed HL to %02x%02x\n", state->h, state->l);
        state->cycles += 10;
        opbytes = 3;
        break;
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
        printf("Moved into H: %02x\n", state->h);
        opbytes = 2;
        state->cycles += 7;
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
        printf("Moved into L: %02x\n", state->l);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0x31:
        // 0x31	LXI SP, D16	(3)		SP.hi <- byte 3, SP.lo <- byte 2
        state->sp = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        printf("Changed SP to %02x\n", state->sp);
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
        printf("Moved into A: %02x\n", state->a);
        opbytes = 2;
        state->cycles += 7;
        break;
    case 0xc2:
        // 0xc2	JNZ adr	3		if NZ, PC <- adr
        if (state->cc.z == 0)
        {
            address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
            printf("Jumping to address on condition NZ: %02x\n", address);
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
        printf("Jumping to address %02x\n", address);
        state->pc = address;
        opbytes = 0;
        state->cycles += 10;
        break;
    case 0xc5:
        // 0xc5	PUSH B	1		(sp-2)<-C; (sp-1)<-B; sp <- sp - 2
        state->memory[state->sp - 2] = state->c;
        state->memory[state->sp - 1] = state->b;
        state->sp -= 2;

        opbytes = 1;
        state->cycles += 3;
        break;
    case 0xc9:
        // 0xc9	RET	1		PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2
        state->pc = (state->memory[state->sp + 1] << 8) + (state->memory[state->sp]);
        printf("Retuning... SP=%02x, SP+1=%02x, restoring %02x%02x to PC\n", (state->sp), (state->sp) + 1, state->memory[state->sp + 1], state->memory[state->sp]);
        state->sp += 2;

        opbytes = 3;
        state->cycles += 10;
        break;
    case 0xcd:
        // 0xcd	CALL adr	3		(SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr
        // using the content of SP as reference address, load PC (2 bytes) into the 2 memory addresses that are before the reference (SP)
        //
        printf("Calling... SP-2=%02x, SP-1=%02x, storing %02x\n", (state->sp) - 2, (state->sp) - 1, state->pc);
        state->memory[(state->sp) - 2] = state->pc & 0xFF; // PC.lo
        state->memory[(state->sp) - 1] = state->pc >> 8;   // PC.hi
        state->sp = (state->sp) - 2;

        address = (state->memory[state->pc + 2] << 8) + state->memory[state->pc + 1];
        state->pc = address;

        printf("\n\nState after call\n\n");
        printf("SP: %02x\n", state->sp);
        printf("PC: %02x\n", state->pc);

        opbytes = 0;
        state->cycles += 17;
        break;

    case 0xd3:
        // state->bus[state->pc + 1] = state->a;
        opbytes = 2;
        state->cycles += 3;
        break;

    case 0xd5:
        // 0xd5	PUSH D	1		(sp-2)<-E; (sp-1)<-D; sp <- sp - 2
        state->memory[state->sp - 2] = state->e;
        state->memory[state->sp - 1] = state->d;
        state->sp -= 2;

        opbytes = 1;
        state->cycles += 3;
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
        state->a = state->a + state->b;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x81:
        // 0x81	ADD C	1	Z, S, P, CY, AC	A <- A + C
        state->a = state->a + state->c;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x82:
        // 0x82	ADD D	1	Z, S, P, CY, AC	A <- A + D
        state->a = state->a + state->d;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x83:
        // 0x83	ADD E	1	Z, S, P, CY, AC	A <- A + E
        state->a = state->a + state->e;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x84:
        // 0x84	ADD H	1	Z, S, P, CY, AC	A <- A + H
        state->a = state->a + state->h;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x85:
        // 0x85	ADD L	1	Z, S, P, CY, AC	A <- A + L
        state->a = state->a + state->l;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0x86:
        // 0x86	ADD M	1	Z, S, P, CY, AC	A <- A + (HL)
        {
            uint16_t hl = (state->h << 8) + (state->l);
            state->a = state->a + hl;
            SetFlags(state, state->a);
            state->cycles += 2;
            opbytes = 1;
            break;
        }
    case 0x87:
        // 0x87	ADD A	1	Z, S, P, CY, AC	A <- A + A
        state->a = state->a + state->a;
        SetFlags(state, state->a);
        state->cycles += 1;
        opbytes = 1;
        break;

        // ANDs
    case 0xa0:
        // 0xa0	ANA B	1	Z, S, P, CY, AC	A <- A & B
        state->a = state->a & state->b;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa1:
        // 0xa1	ANA C	1	Z, S, P, CY, AC	A <- A & C
        state->a = state->a & state->c;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa2:
        // 0xa2	ANA D	1	Z, S, P, CY, AC	A <- A & D
        state->a = state->a & state->d;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa3:
        // 0xa3	ANA E	1	Z, S, P, CY, AC	A <- A & E
        state->a = state->a & state->e;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa4:
        // 0xa4	ANA H	1	Z, S, P, CY, AC	A <- A & H
        state->a = state->a & state->c;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa5:
        // 0xa5	ANA L	1	Z, S, P, CY, AC	A <- A & L
        state->a = state->a & state->l;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa6:
        // 0xa6	ANA M	1	Z, S, P, CY, AC	A <- A & (HL)
        state->a = state->a & state->memory[(state->h << 8) + (state->l)];
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xa7:
        // 0xa7	ANA A	1	Z, S, P, CY, AC	A <- A & A
        state->a = state->a & state->a;
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cycles += 1;
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
        state->a = state->a ^ state->c;
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
        state->cycles += 1;
        opbytes = 1;
        break;
    case 0xae:
        // 0xae	XRA M	1	Z, S, P, CY, AC	A <- A ^ (HL)
        state->a = state->a ^ state->memory[(state->h << 8) + (state->l)];
        SetFlags(state, state->a);

        // clear CY and AC
        state->cc.cy = 0;
        state->cc.ac = 0;
        state->cycles += 1;
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

    case 0xc1:
        // 0xc1	POP B	1		C <- (sp); B <- (sp+1); sp <- sp+2
        state->c = state->memory[state->sp];
        state->b = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;
    case 0xc6:
        // 0xc6	ADI D8	2	Z, S, P, CY, AC	A <- A + byte
        state->a = state->a + state->memory[state->pc + 1];

        SetFlags(state, state->a);

        opbytes = 2;
        state->cycles += 2;
        break;
    case 0xd1:
        // 0xd1	POP D	1		E <- (sp); D <- (sp+1); sp <- sp+2
        state->e = state->memory[state->sp];
        state->d = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;
    case 0xe1:
        // 0xe1	POP H	1		L <- (sp); H <- (sp+1); sp <- sp+2
        state->l = state->memory[state->sp];
        state->h = state->memory[state->sp + 1];
        state->sp += 2;

        state->cycles += 10;
        opbytes = 1;
        break;

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
    case 0xf1:
        // 0xf1	POP PSW	1		flags <- (sp); A <- (sp+1); sp <- sp+2
        {
            uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (state->cc.ac << 4) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);

            state->cc.s = (state->sp >> 7) & 0x01;
            state->cc.z = (state->sp >> 6) & 0x01;
            state->cc.ac = (state->sp >> 4) & 0x01;
            state->cc.p = (state->sp >> 2) & 0x01;
            state->cc.cy = (state->sp & 0x01);

            state->memory[state->sp - 2] = flags;
            state->a = state->memory[state->sp - 1];
            state->sp += 2;

            opbytes = 1;
            state->cycles += 3;
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
    case 0xf5:
        // 0xf5	PUSH PSW	1		(sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
        {
            uint8_t flags = (state->cc.s << 7) + (state->cc.z << 6) + (state->cc.ac << 4) + (state->cc.p << 2) + (1 << 1) + (state->cc.cy);
            state->memory[state->sp - 2] = flags;
            state->memory[state->sp - 1] = state->a;
            state->sp -= 2;

            opbytes = 1;
            state->cycles += 3;
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
    case 0xfe:
    {
        // 0xfe	CPI D8	2	Z, S, P, CY, AC	A - data
        uint8_t comp = state->a - state->memory[state->pc + 1];

        SetFlags(state, comp);

        state->cycles += 7;
        opbytes = 2;
        break;
    }

    default:
        UnimplementedInstruction(state);
    }

    printf("\n");
    return opbytes;
}

int main(int argc, char **argv)
{
    // first, load the code into memory
    State8080 *state = (State8080 *)malloc(sizeof(State8080));

    InitializeRegisters(state);
    InitializeMemory(state);

    int opbytes = 1;
    int instr_count = 0;

    if (!state)
    {
        printf("error: Unable to allocate memory for state.\n");
        exit(1);
    }

    if (!state->memory)
    {
        printf("error: Unable to allocate memory for 8080 memory.\n");
        exit(1);
    }

    FILE *fp = fopen("invaders.rom", "rb");

    if (fp == NULL)
    {
        printf("error: Couldn't open %s\n", argv[1]);
        exit(1);
    }

    // set the pointer at the end of the file to get the size
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);

    // put the cursor back at the beginning
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(fsize);

    fread(buffer, fsize, 1, fp);
    fclose(fp);

    // state->memory = buffer;

    // move the content of the rom to memory
    int i = 0;
    // printf("Memory map \n");
    for (i; i < fsize; i++)
    {
        state->memory[i] = buffer[i];

        // printf("%02x   %02x\n", i, (state->memory[i]));
    }

    // let the engine run
    while (1)
    {

        uint8_t *code = &state->memory[state->pc];
        printf("Instructions ran: %d\n", instr_count);
        opbytes = Emulate8080(state);

        state->pc += opbytes;
        instr_count++;
        opbytes = 1;

        ShowState(state);
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
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

    int pc = 0;
    int opbytes = 1;
    // printf("%c", &buffer[0]);

    while (pc < fsize)
    {
        printf("%x\n", buffer[pc]);

        // const unsigned char *code = &buffer[pc];

        // switch (*code)
        // {
        // case 0x00:
        //     printf("NOP\n");
        //     break;

        // case 0x01:
        //     printf("LXI  B, $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;

        // case 0x02:
        //     printf("STAX  B\n");
        //     break;
        // case 0x03:
        //     printf("INX  B\n");
        //     break;
        // case 0x04:
        //     printf("INR  B\n");
        //     break;
        // case 0x05:
        //     printf("DCR  B\n");
        //     break;
        // case 0x06:
        //     printf("MVI B, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x07:
        //     printf("RLC\n");
        //     break;
        // case 0x08:
        //     printf("-\n");
        //     break;
        // case 0x09:
        //     printf("DAD B\n");
        //     break;
        // case 0x0a:
        //     printf("LDAX B\n");
        //     break;
        // case 0x0b:
        //     printf("DCX B\n");
        //     break;
        // case 0x0c:
        //     printf("INR C\n");
        //     break;
        // case 0x0d:
        //     printf("DCR C\n");
        //     break;
        // case 0x0e:
        //     printf("MVI C, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x0f:
        //     printf("RRC\n");
        //     break;
        // case 0x10:
        //     printf("-\n");
        //     break;
        // case 0x11:
        //     printf("LXI D, $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x12:
        //     printf("STAX D\n");
        //     break;
        // case 0x13:
        //     printf("INX D\n");
        //     break;
        // case 0x14:
        //     printf("INR D\n");
        //     break;
        // case 0x15:
        //     printf("DCR D\n");
        //     break;
        // case 0x16:
        //     printf("MVI D, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x17:
        //     printf("RAL\n");
        //     break;
        // case 0x18:
        //     printf("-\n");
        //     break;
        // case 0x19:
        //     printf("DAD D\n");
        //     break;
        // case 0x1a:
        //     printf("LDAX D\n");
        //     break;
        // case 0x1b:
        //     printf("DCX D\n");
        //     break;
        // case 0x1c:
        //     printf("INR E\n");
        //     break;
        // case 0x1d:
        //     printf("DCR E\n");
        //     break;
        // case 0x1e:
        //     printf("MVI E, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x1f:
        //     printf("RAR\n");
        //     break;
        // case 0x20:
        //     printf("-\n");
        //     break;
        // case 0x21:
        //     printf("LXI H, $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x22:
        //     printf("SHLD $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x23:
        //     printf("INX H");
        //     opbytes = 3;
        //     break;
        // case 0x24:
        //     printf("INR H");
        //     break;
        // case 0x25:
        //     printf("DCR H");
        //     break;
        // case 0x26:
        //     printf("MVI H, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x27:
        //     printf("DAA (special)\n");
        //     break;
        // case 0x28:
        //     printf("-\n");
        //     break;
        // case 0x29:
        //     printf("DAD H\n");
        //     break;
        // case 0x2a:
        //     printf("LHLD $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x2b:
        //     printf("DCX H\n");
        //     break;
        // case 0x2c:
        //     printf("INR L\n");
        //     break;
        // case 0x2d:
        //     printf("DCR L\n");
        //     break;
        // case 0x2e:
        //     printf("MVI L, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x2f:
        //     printf("CMA\n");
        //     break;
        // case 0x30:
        //     printf("-\n");
        //     break;
        // case 0x32:
        //     printf("STA $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x33:
        //     printf("INX SP\n");
        //     break;
        // case 0x34:
        //     printf("INR M\n");
        //     break;
        // case 0x35:
        //     printf("DCR M\n");
        //     break;
        // case 0x36:
        //     printf("MVI M, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x37:
        //     printf("STC\n");
        //     break;
        // case 0x38:
        //     printf("-\n");
        //     break;
        // case 0x39:
        //     printf("DAD SP\n");
        //     break;
        // case 0x3a:
        //     printf("LDA $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0x3b:
        //     printf("DCX SP\n");
        //     break;
        // case 0x3c:
        //     printf("INR A\n");
        //     break;
        // case 0x3d:
        //     printf("DCR A\n");
        //     break;
        // case 0x3e:
        //     printf("MVI A, $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0x3f:
        //     printf("CMC\n");
        //     break;

        // case 0x40:
        //     printf("MOV B,B\n");
        //     break;
        // case 0x41:
        //     printf("MOV B,C\n");
        //     break;
        // case 0x42:
        //     printf("MOV B,D\n");
        //     break;
        // case 0x43:
        //     printf("MOV B,E\n");
        //     break;
        // case 0x44:
        //     printf("MOV B,H\n");
        //     break;
        // case 0x45:
        //     printf("MOV B,L\n");
        //     break;
        // case 0x46:
        //     printf("MOV B,M\n");
        //     break;
        // case 0x47:
        //     printf("MOV B,A\n");
        //     break;

        // case 0x48:
        //     printf("MOV C,B\n");
        //     break;
        // case 0x49:
        //     printf("MOV C,C\n");
        //     break;
        // case 0x4a:
        //     printf("MOV C,D\n");
        //     break;
        // case 0x4b:
        //     printf("MOV C,E\n");
        //     break;
        // case 0x4c:
        //     printf("MOV C,H\n");
        //     break;
        // case 0x4d:
        //     printf("MOV C,L\n");
        //     break;
        // case 0x4e:
        //     printf("MOV C,M\n");
        //     break;
        // case 0x4f:
        //     printf("MOV C,A\n");
        //     break;

        // case 0x50:
        //     printf("MOV D,B\n");
        //     break;
        // case 0x51:
        //     printf("MOV D,C\n");
        //     break;
        // case 0x52:
        //     printf("MOV D,D\n");
        //     break;
        // case 0x53:
        //     printf("MOV D,E\n");
        //     break;
        // case 0x54:
        //     printf("MOV D,H\n");
        //     break;
        // case 0x55:
        //     printf("MOV D,L\n");
        //     break;
        // case 0x56:
        //     printf("MOV D,M\n");
        //     break;
        // case 0x57:
        //     printf("MOV D,A\n");
        //     break;

        // case 0x58:
        //     printf("MOV E,B\n");
        //     break;
        // case 0x59:
        //     printf("MOV E,C\n");
        //     break;
        // case 0x5a:
        //     printf("MOV E,D\n");
        //     break;
        // case 0x5b:
        //     printf("MOV E,E\n");
        //     break;
        // case 0x5c:
        //     printf("MOV E,H\n");
        //     break;
        // case 0x5d:
        //     printf("MOV E,L\n");
        //     break;
        // case 0x5e:
        //     printf("MOV E,M\n");
        //     break;
        // case 0x5f:
        //     printf("MOV E,A\n");
        //     break;

        // case 0x60:
        //     printf("MOV H,B\n");
        //     break;
        // case 0x61:
        //     printf("MOV H,C\n");
        //     break;
        // case 0x62:
        //     printf("MOV H,D\n");
        //     break;
        // case 0x63:
        //     printf("MOV H,E\n");
        //     break;
        // case 0x64:
        //     printf("MOV H,H\n");
        //     break;
        // case 0x65:
        //     printf("MOV H,L\n");
        //     break;
        // case 0x66:
        //     printf("MOV H,M\n");
        //     break;
        // case 0x67:
        //     printf("MOV H,A\n");
        //     break;

        // case 0x68:
        //     printf("MOV L,B\n");
        //     break;
        // case 0x69:
        //     printf("MOV L,C\n");
        //     break;
        // case 0x6a:
        //     printf("MOV L,D\n");
        //     break;
        // case 0x6b:
        //     printf("MOV L,E\n");
        //     break;
        // case 0x6c:
        //     printf("MOV L,H\n");
        //     break;
        // case 0x6d:
        //     printf("MOV L,L\n");
        //     break;
        // case 0x6e:
        //     printf("MOV L,M\n");
        //     break;
        // case 0x6f:
        //     printf("MOV L,A\n");
        //     break;

        // case 0x70:
        //     printf("MOV M,B\n");
        //     break;
        // case 0x71:
        //     printf("MOV M,C\n");
        //     break;
        // case 0x72:
        //     printf("MOV M,D\n");
        //     break;
        // case 0x73:
        //     printf("MOV M,E\n");
        //     break;
        // case 0x74:
        //     printf("MOV M,H\n");
        //     break;
        // case 0x75:
        //     printf("MOV M,L\n");
        //     break;
        // case 0x76:
        //     printf("HLT\n");
        //     break;
        // case 0x77:
        //     printf("MOV M,A\n");
        //     break;

        // case 0x78:
        //     printf("MOV A,B\n");
        //     break;
        // case 0x79:
        //     printf("MOV A,C\n");
        //     break;
        // case 0x7a:
        //     printf("MOV A,D\n");
        //     break;
        // case 0x7b:
        //     printf("MOV A,E\n");
        //     break;
        // case 0x7c:
        //     printf("MOV A,H\n");
        //     break;
        // case 0x7d:
        //     printf("MOV A,L\n");
        //     break;
        // case 0x7e:
        //     printf("MOV A,M\n");
        //     break;
        // case 0x7f:
        //     printf("MOV A,A\n");
        //     break;

        // case 0x80:
        //     printf("ADD B\n");
        //     break;
        // case 0x81:
        //     printf("ADD C\n");
        //     break;
        // case 0x82:
        //     printf("ADD D\n");
        //     break;
        // case 0x83:
        //     printf("ADD E\n");
        //     break;
        // case 0x84:
        //     printf("ADD H\n");
        //     break;
        // case 0x85:
        //     printf("ADD L\n");
        //     break;
        // case 0x86:
        //     printf("ADD M\n");
        //     break;
        // case 0x87:
        //     printf("ADD A\n");
        //     break;

        // case 0x88:
        //     printf("ADC B\n");
        //     break;
        // case 0x89:
        //     printf("ADC C\n");
        //     break;
        // case 0x8a:
        //     printf("ADC D\n");
        //     break;
        // case 0x8b:
        //     printf("ADC E\n");
        //     break;
        // case 0x8c:
        //     printf("ADC H\n");
        //     break;
        // case 0x8d:
        //     printf("ADC L\n");
        //     break;
        // case 0x8e:
        //     printf("ADC M\n");
        //     break;
        // case 0x8f:
        //     printf("ADC A\n");
        //     break;

        // case 0x90:
        //     printf("SUB B\n");
        //     break;
        // case 0x91:
        //     printf("SUB C\n");
        //     break;
        // case 0x92:
        //     printf("SUB D\n");
        //     break;
        // case 0x93:
        //     printf("SUB E\n");
        //     break;
        // case 0x94:
        //     printf("SUB H\n");
        //     break;
        // case 0x95:
        //     printf("SUB L\n");
        //     break;
        // case 0x96:
        //     printf("SUB M\n");
        //     break;
        // case 0x97:
        //     printf("SUB A\n");
        //     break;

        // case 0x98:
        //     printf("SBB B\n");
        //     break;
        // case 0x99:
        //     printf("SBB C\n");
        //     break;
        // case 0x9a:
        //     printf("SBB D\n");
        //     break;
        // case 0x9b:
        //     printf("SBB E\n");
        //     break;
        // case 0x9c:
        //     printf("SBB H\n");
        //     break;
        // case 0x9d:
        //     printf("SBB L\n");
        //     break;
        // case 0x9e:
        //     printf("SBB M\n");
        //     break;
        // case 0x9f:
        //     printf("SBB A\n");
        //     break;

        // case 0xa0:
        //     printf("ANA B\n");
        //     break;
        // case 0xa1:
        //     printf("ANA C\n");
        //     break;
        // case 0xa2:
        //     printf("ANA D\n");
        //     break;
        // case 0xa3:
        //     printf("ANA E\n");
        //     break;
        // case 0xa4:
        //     printf("ANA H\n");
        //     break;
        // case 0xa5:
        //     printf("ANA L\n");
        //     break;
        // case 0xa6:
        //     printf("ANA M\n");
        //     break;
        // case 0xa7:
        //     printf("ANA A\n");
        //     break;

        // case 0xa8:
        //     printf("XRA B\n");
        //     break;
        // case 0xa9:
        //     printf("XRA C\n");
        //     break;
        // case 0xaa:
        //     printf("XRA D\n");
        //     break;
        // case 0xab:
        //     printf("XRA E\n");
        //     break;
        // case 0xac:
        //     printf("XRA H\n");
        //     break;
        // case 0xad:
        //     printf("XRA L\n");
        //     break;
        // case 0xae:
        //     printf("XRA M\n");
        //     break;
        // case 0xaf:
        //     printf("XRA A\n");
        //     break;

        // case 0xb0:
        //     printf("ORA B\n");
        //     break;
        // case 0xb1:
        //     printf("ORA C\n");
        //     break;
        // case 0xb2:
        //     printf("ORA D\n");
        //     break;
        // case 0xb3:
        //     printf("ORA E\n");
        //     break;
        // case 0xb4:
        //     printf("ORA H\n");
        //     break;
        // case 0xb5:
        //     printf("ORA L\n");
        //     break;
        // case 0xb6:
        //     printf("ORA M\n");
        //     break;
        // case 0xb7:
        //     printf("ORA A\n");
        //     break;

        // case 0xb8:
        //     printf("CMP B\n");
        //     break;
        // case 0xb9:
        //     printf("CMP C\n");
        //     break;
        // case 0xba:
        //     printf("CMP D\n");
        //     break;
        // case 0xbb:
        //     printf("CMP E\n");
        //     break;
        // case 0xbc:
        //     printf("CMP H\n");
        //     break;
        // case 0xbd:
        //     printf("CMP L\n");
        //     break;
        // case 0xbe:
        //     printf("CMP M\n");
        //     break;
        // case 0xbf:
        //     printf("CMP A\n");
        //     break;

        // case 0xc0:
        //     printf("RNZ\n");
        //     break;
        // case 0xc1:
        //     printf("POP B\n");
        //     break;
        // case 0xc2:
        //     printf("JNZ $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xc3:
        //     printf("JMP $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xc4:
        //     printf("CNZ $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xc5:
        //     printf("PUSH B\n");
        //     break;
        // case 0xc6:
        //     printf("ADI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xc7:
        //     printf("RST 0\n");
        //     break;

        // case 0xc8:
        //     printf("RZ\n");
        //     break;
        // case 0xc9:
        //     printf("RET\n");
        //     break;
        // case 0xca:
        //     printf("JZ $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xcb:
        //     printf("-\n");
        //     break;
        // case 0xcc:
        //     printf("CZ $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xcd:
        //     printf("CALL $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xce:
        //     printf("ACI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xcf:
        //     printf("RST 1\n");
        //     break;

        // case 0xd0:
        //     printf("RNC\n");
        //     break;
        // case 0xd1:
        //     printf("POP D\n");
        //     break;
        // case 0xd2:
        //     printf("JNC $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xd3:
        //     printf("OUT D8\n");
        //     opbytes = 2;
        //     break;
        // case 0xd4:
        //     printf("CNC $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xd5:
        //     printf("PUSH D\n");
        //     break;
        // case 0xd6:
        //     printf("SUI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xd7:
        //     printf("RST 2\n");
        //     break;

        // case 0xd8:
        //     printf("RC\n");
        //     break;
        // case 0xd9:
        //     printf("-\n");
        //     break;
        // case 0xda:
        //     printf("JC $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xdb:
        //     printf("IN $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xdc:
        //     printf("CC $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xdd:
        //     printf("-\n");
        //     break;
        // case 0xde:
        //     printf("SBI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xdf:
        //     printf("RST 3\n");
        //     break;

        // case 0xe0:
        //     printf("RPO\n");
        //     break;
        // case 0xe1:
        //     printf("POP H\n");
        //     break;
        // case 0xe2:
        //     printf("JPO $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xe3:
        //     printf("XTHL\n");
        //     break;
        // case 0xe4:
        //     printf("CPO $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xe5:
        //     printf("PUSH H\n");
        //     break;
        // case 0xe6:
        //     printf("ANI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xe7:
        //     printf("RST 4\n");
        //     break;

        // case 0xe8:
        //     printf("RPE\n");
        //     break;
        // case 0xe9:
        //     printf("PCHL\n");
        //     break;
        // case 0xea:
        //     printf("JPE $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xeb:
        //     printf("XCHG\n");
        //     break;
        // case 0xec:
        //     printf("CPE $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xed:
        //     printf("-\n");
        //     break;
        // case 0xee:
        //     printf("XRI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xef:
        //     printf("RST 5\n");
        //     break;

        // case 0xf0:
        //     printf("RP\n");
        //     break;
        // case 0xf1:
        //     printf("POP PSW\n");
        //     break;
        // case 0xf2:
        //     printf("JP $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xf3:
        //     printf("DI\n");
        //     break;
        // case 0xf4:
        //     printf("CP $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xf5:
        //     printf("PUSH PSW\n");
        //     break;
        // case 0xf6:
        //     printf("ORI $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xf7:
        //     printf("RST 6\n");
        //     break;

        // case 0xf8:
        //     printf("RM\n");
        //     break;
        // case 0xf9:
        //     printf("SPHL\n");
        //     break;
        // case 0xfa:
        //     printf("JM $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xfb:
        //     printf("EI\n");
        //     break;
        // case 0xfc:
        //     printf("CM  $%02x%02x\n", code[2], code[1]);
        //     opbytes = 3;
        //     break;
        // case 0xfd:
        //     printf("-\n");
        //     break;
        // case 0xfe:
        //     printf("CPI  $%02x\n", code[1]);
        //     opbytes = 2;
        //     break;
        // case 0xff:
        //     printf("RST 7\n");
        //     break;

        // default:
        //     printf("%02x not implemented yet", *code);
        //     printf("\n");
        // }
        pc += opbytes;
        opbytes = 1;
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ConditionCodes{
	uint8_t z:1;
	uint8_t s:1;
	uint8_t p:1;
	uint8_t cy:1;
	uint8_t ac:1;
	uint8_t pad:3;
	uint8_t interrupt_enabled:1;
} ConditionCodes;

typedef struct State8080{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t h;
	uint8_t l;
	uint16_t sp;
	uint16_t pc;
	uint8_t *memory;
	struct ConditionCodes cc;
	uint8_t int_enable;
} State8080;

void UnimplementedInstruction(State8080* state){

	state->pc -= 1;
	printf("Error: unimplemented instruction\n");
	exit(1);

}

int zspflag(State8080* state, uint16_t answer){
	
	if((answer & 0xff) == 0){
		state->cc.z = 1;
	}else{
		state->cc.z = 0;
	}

	if(answer & 0x80){
		state->cc.s = 1;
	}else{
		state->cc.s = 0;
	}

	if(answer % 2 == 0){
		state->cc.p = 1;
	}else{
		state->cc.p = 0;
	}

	return 0;
}

int zspcyflag(State8080* state, uint16_t answer){

	if((answer & 0xff) == 0){
		state->cc.z = 1;
	}else{
		state->cc.z = 0;
	}

	if(answer & 0x80){
		state->cc.s = 1;
	}else{
		state->cc.s = 0;
	}

	if(answer % 2 == 0){
		state->cc.p = 1;
	}else{
		state->cc.p = 0;
	}

	state->cc.cy = (answer > 0xff);

	return 0;
}

uint8_t inr(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)value + 1;
	zspflag(state, answer);
	return (uint8_t)answer;	

}

uint8_t dcr(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)value - 1;
	zspflag(state, answer);
	return (uint8_t)answer;

}

uint8_t add(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a + (uint16_t)value;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t adc(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a + (uint16_t)value + (uint16_t)state->cc.cy;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t sub(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a - (uint16_t)value;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t sbb(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a - (uint16_t)value - (uint16_t)state->cc.cy;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t ana(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a & (uint16_t)value;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t xra(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a ^ (uint16_t)value;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

uint8_t ora(State8080* state, uint8_t value){

	uint16_t answer = (uint16_t)state->a | (uint16_t)value;
	zspcyflag(state, answer);
	return (uint8_t)answer;

}

int call(State8080* state, unsigned char* opcode){

	uint16_t ret = state->pc + 2;
	state->memory[state->sp-1] = (ret >> 8) & 0xff;
	state->memory[state->sp-2] = (ret & 0xff);
	state->sp = state->sp - 2;
	state->pc = (opcode[2] << 8) | opcode[1];
	return 0;

}

int ret(State8080* state){

	state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
	state->sp += 2;
	return 0;

}

int cmp(State8080* state, uint8_t value){

	uint8_t x = state->a - value;
	zspflag(state, x);
	state->cc.cy = (state->a < value);
	return 0;

}

uint16_t hl(State8080* state){

	return (state->h << 8) | state->l;

}
		
void Emulate8080Op(State8080* state){

	unsigned char *opcode = &state->memory[state->pc];

	switch(*opcode){
		case 0x00:{
			break;
		}
		case 0x01:{
			state->c = opcode[1];
			state->b = opcode[2];
			state->pc += 2;
			break;
		}
		case 0x04:{
			state->b = inr(state, state->b);
			break;
		}
		case 0x05:{
			state->b = dcr(state, state->b);
			break;
		}
		case 0x06:
			state->b = opcode[1];
			state->pc += 1;
			break;
		case 0x07:{
			uint8_t x = state->a;
			state->cc.cy = state->a >> 7;
			state->a = (x << 1) | (state->cc.cy);
			break;
		}	
		case 0x0c:{
			state->c = inr(state, state->c);
			break;
		}
		case 0x0d:{
			state->c = dcr(state, state->c);
			break;
		}
		case 0x0e:
			state->c = opcode[1];
			state->pc += 1;
			break;
		case 0x0f:{
			uint8_t x = state->a;
			state->a = (x >> 1) | ((x & 1) << 7);
			state->cc.cy = ((x & 1) == 1);
			break;
		}
		case 0x11:
			state->d = opcode[2];
			state->e = opcode[1];
			state->pc += 2;
			break;
		case 0x14:
			state->d = inr(state, state->d);
			break;
		case 0x15:
			state->d = dcr(state, state->d);
			break;
		case 0x16:
			state->d = opcode[1];
			state->pc += 1;
			break;
		case 0x17:{
			uint8_t x = state->a;
			state->a = (state->a << 1) | state->cc.cy;
			state->cc.cy = (x >> 7);
			break;
		}
		case 0x1c:
			state->e = inr(state, state->e);
			break;
		case 0x1d:
			state->e = dcr(state, state->e);
			break;
		case 0x1e:
			state->e = opcode[1];
			state->pc += 1;
			break;
		case 0x1f:{
			uint8_t x = state->a;
			state->a = (x >> 1) | (state->cc.cy << 7);
			state->cc.cy = ((x & 1) == 1);
			break;
		}
		case 0x21:
			state->h = opcode[2];
			state->l = opcode[1];
			state->pc += 2;
			break;
		case 0x24:
			state->h = inr(state, state->h);
			break;
		case 0x25:
			state->h = dcr(state, state->h);
			break;
		case 0x26:
			state->h = opcode[1];
			state->pc += 1;
			break;
		case 0x2c:
			state->l = inr(state, state->l);
			break;
		case 0x2d:
			state->l = dcr(state, state->l);
			break;
		case 0x2e:
			state->l = opcode[1];
			state->pc += 1;
			break;
		case 0x2f:
			state->a = ~(state->a);
			break;
		case 0x33:
			state->sp = state->sp + 1;
			break;
		case 0x37:
			state->cc.cy = 1;
			break;
		case 0x3b:
			state->sp = state->sp - 1;
			break;
		case 0x3c:
			state->a = inr(state, state->a);
			break;
		case 0x3d:
			state->a = dcr(state, state->a);
			break;
		case 0x3e:
			state->a = opcode[1];
			state->pc += 1;
			break;
		case 0x3f:
			state->cc.cy = ~(state->cc.cy);
			break;
		case 0x40:
			state->b = state->b;
			break;
		case 0x41:
			state->b = state->c;
			break;
		case 0x42:
			state->b = state->d;
			break;
		case 0x43:
			state->b = state->e;
			break;
		case 0x44:
			state->b = state->h;
			break;
		case 0x45:
			state->b = state->l;
			break;
		case 0x47:
			state->b = state->a;
			break;
		case 0x48:
			state->c = state->b;
			break;
		case 0x49:
			state->c = state->c;
			break;
		case 0x4a:
			state->c = state->d;
			break;
		case 0x4b:
			state->c = state->e;
			break;
		case 0x4c:
			state->c = state->h;
			break;
		case 0x4d:
			state->c = state->l;
			break;
		case 0x4f:
			state->c = state->a;
			break;
		case 0x50:
			state->d = state->b;
			break;
		case 0x51:
			state->d = state->c;
			break;
		case 0x52:
			state->d = state->d;
			break;
		case 0x53:
			state->d = state->e;
			break;
		case 0x54:
			state->d = state->h;
			break;
		case 0x55:
			state->d = state->l;
			break;
		case 0x57:
			state->d = state->a;
			break;
		case 0x58:
			state->e = state->b;
			break;
		case 0x59:
			state->e = state->c;
			break;
		case 0x5a:
			state->e = state->d;
			break;
		case 0x5b:
			state->e = state->e;
			break;
		case 0x5c:
			state->e = state->h;
			break;
		case 0x5d:
			state->e = state->l;
			break;
		case 0x5f:
			state->e = state->a;
			break;
		case 0x60:
			state->h = state->b;
			break;
		case 0x61:
			state->h = state->c;
			break;
		case 0x62:
			state->h = state->d;
			break;
		case 0x63:
			state->h = state->e;
			break;
		case 0x64:
			state->h = state->h;
			break;
		case 0x65:
			state->h = state->l;
			break;
		case 0x67:
			state->h = state->a;
			break;
		case 0x68:
			state->l = state->b;
			break;
		case 0x69:
			state->l = state->c;
			break;
		case 0x6a:
			state->l = state->d;
			break;
		case 0x6b:
			state->l = state->e;
			break;
		case 0x6c:
			state->l = state->h;
			break;
		case 0x6d:
			state->l = state->l;
			break;
		case 0x6f:
			state->l = state->a;
			break;
		case 0x76:
			exit(0);
			break;
		case 0x78:
			state->a = state->b;
			break;
		case 0x79:
			state->a = state->c;
			break;
		case 0x7a:
			state->a = state->d;
			break;
		case 0x7b:
			state->a = state->e;
			break;
		case 0x7c:
			state->a = state->h;
			break;
		case 0x7d:
			state->a = state->l;
			break;
		case 0x7f:
			state->a = state->a;
			break;
		case 0x80:
			state->a = add(state, state->b);
			break;
		case 0x81:
			state->a = add(state, state->c);
			break;
		case 0x82:
			state->a = add(state, state->d);
			break;
		case 0x83:
			state->a = add(state, state->e);
			break;
		case 0x84:
			state->a = add(state, state->h);
			break;
		case 0x85:
			state->a = add(state, state->l);
			break;
		case 0x87:
			state->a = add(state, state->a);
			break;
		case 0x88:
			state->a = adc(state, state->b);
			break;
		case 0x89:
			state->a = adc(state, state->c);
			break;
		case 0x8a:
			state->a = adc(state, state->d);
			break;
		case 0x8b:
			state->a = adc(state, state->e);
			break;
		case 0x8c:
			state->a = adc(state, state->h);
			break;
		case 0x8d:
			state->a = adc(state, state->l);
			break;
		case 0x8f:
			state->a = adc(state, state->a);
			break;
		case 0x90:
			state->a = sub(state, state->b);
			break;
		case 0x91:
			state->a = sub(state, state->c);
			break;
		case 0x92:
			state->a = sub(state, state->d);
			break;
		case 0x93:
			state->a = sub(state, state->e);
			break;
		case 0x94:
			state->a = sub(state, state->h);
			break;
		case 0x95:
			state->a = sub(state, state->l);
			break;
		case 0x97:
			state->a = sub(state, state->a);
			break;
		case 0x98:
			state->a = sbb(state, state->b);
			break;
		case 0x99:
			state->a = sbb(state, state->c);
			break;
		case 0x9a:
			state->a = sbb(state, state->d);
			break;
		case 0x9b:
			state->a = sbb(state, state->e);
			break;
		case 0x9c:
			state->a = sbb(state, state->h);
			break;
		case 0x9d:
			state->a = sbb(state, state->l);
			break;
		case 0x9f:
			state->a = sbb(state, state->a);
			break;
		case 0xa0:
			state->a = ana(state, state->b);
			break;
		case 0xa1:
			state->a = ana(state, state->c);
			break;
		case 0xa2:
			state->a = ana(state, state->d);
			break;
		case 0xa3:
			state->a = ana(state, state->e);
			break;
		case 0xa4:
			state->a = ana(state, state->h);
			break;
		case 0xa5:
			state->a = ana(state, state->l);
			break;
		case 0xa7:
			state->a = ana(state, state->a);
			break;
		case 0xa8:
			state->a = xra(state, state->b);
			break;
		case 0xa9:
			state->a = xra(state, state->c);
			break;
		case 0xaa:
			state->a = xra(state, state->d);
			break;
		case 0xab:
			state->a = xra(state, state->e);
			break;
		case 0xac:
			state->a = xra(state, state->h);
			break;
		case 0xad:
			state->a = xra(state, state->l);
			break;
		case 0xaf:
			state->a = xra(state, state->a);
			break;
		case 0xb0:
			state->a = ora(state, state->b);
			break;
		case 0xb1:
			state->a = ora(state, state->c);
			break;
		case 0xb2:
			state->a = ora(state, state->d);
			break;
		case 0xb3:
			state->a = ora(state, state->e);
			break;
		case 0xb4:
			state->a = ora(state, state->h);
			break;
		case 0xb5:
			state->a = ora(state, state->l);
			break;
		case 0xb7:
			state->a = ora(state, state->a);
			break;
		case 0xb8:
			cmp(state, state->b);
			break;
		case 0xb9:
			cmp(state, state->c);
			break;
		case 0xba:
			cmp(state, state->d);
			break;
		case 0xbb:
			cmp(state, state->e);
			break;
		case 0xbc:
			cmp(state, state->h);
			break;
		case 0xbd:
			cmp(state, state->l);
			break;
		case 0xbf:
			cmp(state, state->a);
			break;
		case 0xc0:
			if (state->cc.z == 0){
				ret(state);
			}
			break;
		case 0xc1:
			state->c = state->memory[state->sp];
			state->b = state->memory[state->sp + 1];
			state->sp += 2;
			break;
		case 0xc2:
			if (0 == state->cc.z){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xc3:
			state->pc = (opcode[2] << 8) | opcode[1];
			break;
		case 0xc4:
			if (state->cc.z == 0){
				call(state, opcode);				
			}else{
				state->pc += 2;
			}
			break;
		case 0xc5:
			state->memory[state->sp - 2] = state->c;
			state->memory[state->sp - 1] = state->b;
			state->sp -= 2;
			break;
		case 0xc8:
			if (state->cc.z){
				ret(state);
			}
			break;
		case 0xc9:
			ret(state);
			break;
		case 0xca:
			if (state->cc.z){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xcc:
			if (state->cc.z){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xcd:
			call(state, opcode);
			break;
		case 0xd0:
			if (state->cc.cy == 0){
				ret(state);
			}
			break;
		case 0xd1:
			state->e = state->memory[state->sp];
			state->d = state->memory[state->sp + 1];
			state->sp += 2;
			break;
		case 0xd2:
			if (0 == state->cc.cy){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xd3:
			state->pc += 1;
			break;
		case 0xd4:
			if (0 == state->cc.cy){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xd5:
			state->memory[state->sp - 2] = state->e;
			state->memory[state->sp - 1] = state->d;
			state->sp -= 2;
			break;
		case 0xd8:
			if (state->cc.cy){
				ret(state);
			}
			break;
		case 0xda:
			if (state->cc.cy){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xdb:
			state->pc += 1;
			break;
		case 0xdc:
			if (state->cc.cy){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xe0:
			if (state->cc.p == 0){
				ret(state);
			}
			break;
		case 0xe1:
			state->l = state->memory[state->sp];
			state->h = state->memory[state->sp + 1];
			state->sp += 2;
			break;
		case 0xe2:
			if (state->cc.p == 0){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xe3:{
			uint8_t l = state->l;
			uint8_t h = state->h;
			state->l = state->memory[state->sp];
			state->h = state->memory[state->sp + 1];
			state->memory[state->sp] = l;
			state->memory[state->sp + 1] = h;
			break;
		}
		case 0xe4:
			if (state->cc.p == 0){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xe5:
			state->memory[state->sp - 2] = state->l;
			state->memory[state->sp - 1] = state->h;
			state->sp -= 2;
			break;
		case 0xe6:{
			uint8_t answer = state->a & opcode[1];
			state->cc.z = (answer == 0);
			state->cc.s = (0x80 == (answer & 0x80));
			if (answer % 2 == 0){
				state->cc.p = 1;
			}else{
				state->cc.p = 0;
			}
			state->cc.cy = 0;
			state->a = answer;
			state->pc += 1;
			break;		
		}
		case 0xe8:
			if (state->cc.p){
				ret(state);
			}
			break;
		case 0xea:
			if (state->cc.p){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xec:
			if (state->cc.p){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xf0:
			if (state->cc.p){
				ret(state);
			}
			break;
		case 0xf1:{
			uint8_t psw = state->memory[state->sp];
			state->cc.z = ((psw & 0x01) == 0x01);
			state->cc.s = ((psw & 0x02) == 0x02);
			state->cc.p = ((psw & 0x04) == 0x04);
			state->cc.cy = ((psw & 0x08) == 0x05);
			state->cc.ac = ((psw & 0x10) == 0x10);
			state->a = state->memory[state->sp + 1];
			state->sp += 2;
			break;
		}
		case 0xf2:
			if (state->cc.s == 0){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xf3:
			state->cc.interrupt_enabled = 0;
			break;
		case 0xf5:{
			uint8_t psw = (state->cc.z << 1 | state->cc.p << 2 | state->cc.cy << 3 | state->cc.ac << 4);
			state->memory[state->sp - 2] = psw;
			state->memory[state->sp - 1] = state->a;
			state->sp -= 2;
			break;
		}
		case 0xf6:
			state->a = ora(state, opcode[1]);
			break;
		case 0xf8:
			if (state->cc.s){
				ret(state);
			}
			break;
		case 0xf9:
			state->sp = (state->h << 8 | state->l);
			break;
		case 0xfa:
			if (state->cc.s){
				state->pc = (opcode[2] << 8) | opcode[1];
			}else{
				state->pc += 2;
			}
			break;
		case 0xfb:
			state->cc.interrupt_enabled = 1;
			break;
		case 0xfc:
			if (state->cc.s){
				call(state, opcode);
			}else{
				state->pc += 2;
			}
			break;
		case 0xfe:{
			uint8_t x = state->a - opcode[1];
			state->cc.z = (x == 0);
			state->cc.s = ((x & 0x80) == 0x80);
			if (x % 2 == 0){
				state->cc.p = 1;
			}else{
				state->cc.p = 0;
			}
			state->cc.cy = (state->a < opcode[1]);
			state->pc += 1;
			break;
		}
	}
	
	state->pc += 1;
}



/*
 *codebuffer is pointer to 8080 assembly code
 pc is the current offset of codebuffer pointer
 
 returns number of bytes required by opcode
*/

int disassemble8080Op(unsigned char *codebuffer, int pc){

	unsigned char *code = &codebuffer[pc];
	int opbytes = 1;
	printf("%04x ", pc);
	switch(*code){
		case 0x00:
			printf("NOP");
			break;
		case 0x01:
			printf("LXI	B, #$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x02:
			printf("STAX	B");
			break;
		case 0x03:
			printf("INX	B");
			break;
		case 0x04:
			printf("INR	B");
			break;
		case 0x05:
			printf("DCR	B");
			break;
		case 0x06:
			printf("MVI	B, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x07:
			printf("RLC");
			break;
		case 0x08:
			printf("NOP");
			break;
		case 0x09:
			printf("DAD	B");
			break;
		case 0x0a:
			printf("LDAX	B");
			break;
		case 0x0b:
			printf("DCX	B");
			break;
		case 0x0c:
			printf("INR	C");
			break;
		case 0x0d:
			printf("DCR	C");
			break;
		case 0x0e:
			printf("MVI	C, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x0f:
			printf("RRC");
			break;
		case 0x10:
			printf("NOP");
			break;
		case 0x11:
			printf("LXI	D, #$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x12:
			printf("STAX	D");
			break;
		case 0x13:
			printf("INX	D");
			break;
		case 0x14:
			printf("INR	D");
			break;
		case 0x15:
			printf("DCR	D");
			break;
		case 0x16:
			printf("MVI	D, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x17:
			printf("RAL");
			break;
		case 0x18:
			printf("NOP");
			break;
		case 0x19:
			printf("DAD 	D");
			break;
		case 0x1a:
			printf("LDAX	D");
			break;
		case 0x1b:
			printf("DCX	D");
			break;
		case 0x1c:
			printf("INR	E");
			break;
		case 0x1d:
			printf("DCR	E");
			break;
		case 0x1e:
			printf("MVI	E, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x1f:
			printf("RAR");
			break;
		case 0x20:
			printf("RIM");
			break;
		case 0x21:
			printf("LXI	H, #$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x22:
			printf("SHLD	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x23:
			printf("INX	H");
			break;
		case 0x24:
			printf("INR	H");
			break;
		case 0x25:
			printf("DCR	H");
			break;
		case 0x26:
			printf("MVI	H, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x27:
			printf("DAA");
			break;
		case 0x28:
			printf("NOP");
			break;
		case 0x29:
			printf("DAD	H");
			break;
		case 0x2a:
			printf("LHLD	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x2b:
			printf("DCX	H");
			break;
		case 0x2c:
			printf("INR	L");
			break;
		case 0x2d:
			printf("DCR	L");
			break;
		case 0x2e:
			printf("MVI	L, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x2f:
			printf("CMA");
			break;
		case 0x30:
			printf("SIM");
			break;
		case 0x31:
			printf("LXI	SP, #$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x32:
			printf("STA	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x33:
			printf("INX	SP");
			break;
		case 0x34:
			printf("INR	M");
			break;
		case 0x35:
			printf("DCR	M");
			break;
		case 0x36:
			printf("MVI	M, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x37:
			printf("STC");
			break;
		case 0x38:
			printf("NOP");
			break;
		case 0x39:
			printf("DAD	SP");
			break;
		case 0x3a:
			printf("LDA	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0x3b:
			printf("DCX	SP");
			break;
		case 0x3c:
			printf("INR	A");
			break;
		case 0x3d:
			printf("DCR	A");
			break;
		case 0x3e:
			printf("MVI	A, #$%02x", code[1]);
			opbytes = 2;
			break;
		case 0x3f:
			printf("CMC");
			break;
		case 0x40:
			printf("MOV	B, B");
			break;
		case 0x41:
			printf("MOV	B, C");
			break;
		case 0x42:
			printf("MOV	B, D");
			break;
		case 0x43:
			printf("MOV	B, E");
			break;
		case 0x44:
			printf("MOV	B, H");
			break;
		case 0x45:
			printf("MOV	B, L");
			break;
		case 0x46:
			printf("MOV	B, M");
			break;
		case 0x47:
			printf("MOV	B, A");
			break;
		case 0x48:
			printf("MOV	C, B");
			break;
		case 0x49:
			printf("MOV	C, C");
			break;
		case 0x4a:
			printf("MOV	C, D");
			break;
		case 0x4b:
			printf("MOV	C, E");
			break;
		case 0x4c:
			printf("MOV	C, H");
			break;
		case 0x4d:
			printf("MOV	C, L");
			break;
		case 0x4e:
			printf("MOV	C, M");
			break;
		case 0x4f:
			printf("MOV	C, A");
			break;
		case 0x50:
			printf("MOV	D, B");
			break;
		case 0x51:
			printf("MOV	D, C");
			break;
		case 0x52:
			printf("MOV	D, D");
			break;
		case 0x53:
			printf("MOV	D, E");
			break;
		case 0x54:
			printf("MOV	D, H");
			break;
		case 0x55:
			printf("MOV	D, L");
			break;
		case 0x56:
			printf("MOV	D, M");
			break;
		case 0x57:
			printf("MOV	D, A");
			break;
		case 0x58:
			printf("MOV	E, B");
			break;
		case 0x59:
			printf("MOV	E, C");
			break;
		case 0x5a:
			printf("MOV	E, D");
			break;
		case 0x5b:
			printf("MOV	E, E");
			break;
		case 0x5c:
			printf("MOV	E, H");
			break;
		case 0x5d:
			printf("MOV	E, L");
			break;
		case 0x5e:
			printf("MOV	E, M");
			break;
		case 0x5f:
			printf("MOV	E, A");
			break;
		case 0x60:
			printf("MOV	H, B");
			break;
		case 0x61:
			printf("MOV	H, C");
			break;
		case 0x62:
			printf("MOV	H, D");
			break;
		case 0x63:
			printf("MOV	H, E");
			break;
		case 0x64:
			printf("MOV	H, H");
			break;
		case 0x65:
			printf("MOV	H, L");
			break;
		case 0x66:
			printf("MOV	H, M");
			break;
		case 0x67:
			printf("MOV	H, A");
			break;
		case 0x68:
			printf("MOV	L, B");
			break;
		case 0x69:
			printf("MOV	L, C");
			break;
		case 0x6a:
			printf("MOV	L, D");
			break;
		case 0x6b:
			printf("MOV	L, E");
			break;
		case 0x6c:
			printf("MOV	L, H");
			break;
		case 0x6d:
			printf("MOV	L, L");
			break;
		case 0x6e:
			printf("MOV	L, M");
			break;
		case 0x6f:
			printf("MOV	L, A");
			break;
		case 0x70:
			printf("MOV	M, B");
			break;
		case 0x71:
			printf("MOV	M, C");
			break;
		case 0x72:
			printf("MOV	M, D");
			break;
		case 0x73:
			printf("MOV	M, E");
			break;
		case 0x74:
			printf("MOV	M, H");
			break;
		case 0x75:
			printf("MOV	M, L");
			break;
		case 0x76:
			printf("HLT");
			break;
		case 0x77:
			printf("MOV	M, A");
			break;
		case 0x78:
			printf("MOV	A, B");
			break;
		case 0x79:
			printf("MOV	A, C");
			break;
		case 0x7a:
			printf("MOV	A, D");
			break;
		case 0x7b:
			printf("MOV	A, E");
			break;
		case 0x7c:
			printf("MOV	A, H");
			break;
		case 0x7d:
			printf("MOV	A, L");
			break;
		case 0x7e:
			printf("MOV	A, M");
			break;
		case 0x7f:
			printf("MOV	A, A");
			break;
		case 0x80:
			printf("ADD	B");
			break;
		case 0x81:
			printf("ADD	C");
			break;
		case 0x82:
			printf("ADD	D");
			break;
		case 0x83:
			printf("ADD	E");
			break;
		case 0x84:
			printf("ADD	H");
			break;
		case 0x85:
			printf("ADD	L");
			break;
		case 0x86:
			printf("ADD	M");
			break;
		case 0x87:
			printf("ADD	A");
			break;
		case 0x88:
			printf("ADC	B");
			break;
		case 0x89:
			printf("ADC	C");
			break;
		case 0x8a:
			printf("ADC	D");
			break;
		case 0x8b:
			printf("ADC	E");
			break;
		case 0x8c:
			printf("ADC	H");
			break;
		case 0x8d:
			printf("ADC	L");
			break;
		case 0x8e:
			printf("ADC	M");
			break;
		case 0x8f:
			printf("ADC	A");
			break;
		case 0x90:
			printf("SUB	B");
			break;
		case 0x91:
			printf("SUB	C");
			break;
		case 0x92:
			printf("SUB	D");
			break;
		case 0x93:
			printf("SUB	E");
			break;
		case 0x94:
			printf("SUB	H");
			break;
		case 0x95:
			printf("SUB	L");
			break;
		case 0x96:
			printf("SUB	M");
			break;
		case 0x97:
			printf("SUB	A");
			break;
		case 0x98:
			printf("SBB	B");
			break;
		case 0x99:
			printf("SBB	C");
			break;
		case 0x9a:
			printf("SBB	D");
			break;
		case 0x9b:
			printf("SBB	E");
			break;
		case 0x9c:
			printf("SBB	H");
			break;
		case 0x9d:
			printf("SBB	L");
			break;
		case 0x9e:
			printf("SBB	M");
			break;
		case 0x9f:
			printf("SBB	A");
			break;
		case 0xa0:
			printf("ANA	B");
			break;
		case 0xa1:
			printf("ANA	C");
			break;
		case 0xa2:
			printf("ANA	D");
			break;
		case 0xa3:
			printf("ANA	E");
			break;
		case 0xa4:
			printf("ANA	H");
			break;
		case 0xa5:
			printf("ANA	L");
			break;
		case 0xa6:
			printf("ANA	M");
			break;
		case 0xa7:
			printf("ANA	A");
			break;
		case 0xa8:
			printf("XRA	B");
			break;
		case 0xa9:
			printf("XRA	C");
			break;
		case 0xaa:
			printf("XRA	D");
			break;
		case 0xab:
			printf("XRA	E");
			break;
		case 0xac:
			printf("XRA	H");
			break;
		case 0xad:
			printf("XRA	L");
			break;
		case 0xae:
			printf("XRA	M");
			break;
		case 0xaf:
			printf("XRA	A");
			break;
		case 0xb0:
			printf("ORA	B");
			break;
		case 0xb1:
			printf("ORA	C");
			break;
		case 0xb2:
			printf("ORA	D");
			break;
		case 0xb3:
			printf("ORA	E");
			break;
		case 0xb4:
			printf("ORA	H");
			break;
		case 0xb5:
			printf("ORA	L");
			break;
		case 0xb6:
			printf("ORA	M");
			break;
		case 0xb7:
			printf("ORA	A");
			break;
		case 0xb8:
			printf("CMP	B");
			break;
		case 0xb9:
			printf("CMP	C");
			break;
		case 0xba:
			printf("CMP	D");
			break;
		case 0xbb:
			printf("CMP	E");
			break;
		case 0xbc:
			printf("CMP	H");
			break;
		case 0xbd:
			printf("CMP	L");
			break;
		case 0xbe:
			printf("CMP	M");
			break;
		case 0xbf:
			printf("CMP	A");
			break;
		case 0xc0:
			printf("RNZ");
			break;
		case 0xc1:
			printf("POP	B");
			break;
		case 0xc2:
			printf("JNZ	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xc3:
			printf("JMP	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xc4:
			printf("CNZ	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xc5:
			printf("PUSH	B");
			break;
		case 0xc6:
			printf("ADI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xc7:
			printf("RST	0");
			break;
		case 0xc8:
			printf("RZ");
			break;
		case 0xc9:
			printf("RET");
			break;
		case 0xca:
			printf("JZ	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xcb:
			printf("NOF");
			break;
		case 0xcc:
			printf("CZ	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xcd:
			printf("CALL	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xce:
			printf("ACI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xcf:
			printf("RST	1");
			break;
		case 0xd0:
			printf("RNC");
			break;
		case 0xd1:
			printf("POP	D");
			break;
		case 0xd2:
			printf("JNC	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xd3:
			printf("OUT	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xd4:
			printf("CNC	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xd5:
			printf("PUSH	D");
			break;
		case 0xd6:
			printf("SUI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xd7:
			printf("RST	2");
			break;
		case 0xd8:
			printf("RC");
			break;
		case 0xd9:
			printf("NOP");
			break;
		case 0xda:
			printf("JC	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xdb:
			printf("IN	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xdc:
			printf("CC	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xdd:
			printf("NOP");
			break;
		case 0xde:
			printf("SBI	$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xdf:
			printf("RST	3");
			break;
		case 0xe0:
			printf("RPO");
			break;
		case 0xe1:
			printf("POP	H");
			break;
		case 0xe2:
			printf("JPO	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xe3:
			printf("XTHL");
			break;
		case 0xe4:
			printf("CPO	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xe5:
			printf("PUSH	H");
			break;
		case 0xe6:
			printf("ANI	$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xe7:
			printf("RST	4");
			break;
		case 0xe8:
			printf("RPE");
			break;
		case 0xe9:
			printf("PCHL");
			break;
		case 0xea:
			printf("JPE	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xeb:
			printf("XCHG");
			break;
		case 0xec:
			printf("CPE	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xed:
			printf("NOP");
			break;
		case 0xee:
			printf("XRI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xef:
			printf("RST	5");
			break;
		case 0xf0:
			printf("RP");
			break;
		case 0xf1:
			printf("POP	PSW");
			break;
		case 0xf2:
			printf("JP	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xf3:
			printf("DI");
			break;
		case 0xf4:
			printf("CP	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xf5:
			printf("PUSH	PSW");
			break;
		case 0xf6:
			printf("ORI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xf7:
			printf("RST	6");
			break;
		case 0xf8:
			printf("RM");
			break;
		case 0xf9:
			printf("SPHL");
			break;
		case 0xfa:
			printf("JM	$%02x%02X", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xfb:
			printf("EI");
			break;
		case 0xfc:
			printf("CM	$%02x%02x", code[2], code[1]);
			opbytes = 3;
			break;
		case 0xfd:
			printf("NOP");
			break;
		case 0xfe:
			printf("CPI	#$%02x", code[1]);
			opbytes = 2;
			break;
		case 0xff:
			printf("RST	7");
			break;	
	}
	printf("\n");

	return opbytes;
}

int main(int argc, char** argv){

	FILE *f = fopen(argv[1], "rb");
	if (f == NULL){
		printf("error opening file");
		exit(1);
	}

	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);

	unsigned char *buffer = malloc(fsize);
	if (buffer == NULL){
		printf("error malloc");
		exit(1);
	}	

	fread(buffer, fsize, 1, f);
	fclose(f);

	int pc = 0;

	while(pc < fsize){
		pc += disassemble8080Op(buffer, pc);
	}

	return 0;

}

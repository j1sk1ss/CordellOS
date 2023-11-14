#include "../../include/asm_compiler.h"


extern int symbol_index;
extern int intermediate_index;
extern int blocks_index;
extern struct intermediate_lang **intermediate_table;
extern struct symbol_table **symbol_tab;
extern struct blocks_table **block_tab;

void display_symbol_table() {
	printf("\n---------------Symbol Table is ----------\n");
	for (int i = 0; i < symbol_index; i++)
		printf("\nvariable name is %s,address is %d, size is %d", symbol_tab[i]->variable_name, symbol_tab[i]->address, symbol_tab[i]->size);

	return;
}

void display_intermediate_table() {
	printf("\n---------------Instruction Table is ----------\n");
	for (int i = 0; i < intermediate_index; i++){
		printf("\n%d : %d : ", intermediate_table[i]->instruc_no, intermediate_table[i]->opcode);
		for (int j = 0; intermediate_table[i]->parameters[j] != -1; j++)
			printf(" %d", intermediate_table[i]->parameters[j]);
	}

	printf("\n");
	return;
}

void display_block_table() {
	printf("\n---------------Block Table is ----------\n");
	for (int i = 0; i < blocks_index; i++)
		printf("\n Label is : %s and address is : %d ", block_tab[i]->name, block_tab[i]->instr_no);

	printf("\n");
	return;
}

int check_condition(int operand1, int operand2, int opcode){
	switch(opcode){
		case 8: 
			if (operand1 == operand2)
				return 1;
		break;

		case 9: 
			if (operand1 < operand2)
				return 1;
		break;

		case 10: 
			if (operand1 > operand2)
				return 1;
		break;

		case 11: 
			if (operand1 <= operand2)
				return 1;
		break;

		case 12: 
			if (operand1 >= operand2)
				return 1;
		break;
	}
	
	return 0;
}

void asm_executor(int *memory_array, int memory_index, struct User* user) {
	for (int i = 0; i < intermediate_index;) {  // iterating on the intermediate language table
		switch (intermediate_table[i]->opcode) {
			case 14:  
				printf("\n\r");
                memory_array[intermediate_table[i]->parameters[0]] = atoi(keyboard_read(VISIBLE_KEYBOARD, -1));
			break; // READ Instruction //

			case 1: 
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]]; 
		    break; // MOVE Instruction //

			case 3: 
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]] +
																		 memory_array[intermediate_table[i]->parameters[2]];
			break; // ADD Instruction //

			case 4: 
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]] -
																		 memory_array[intermediate_table[i]->parameters[2]];
		    break; // SUB Instruction //

			case 13: 
                printf("\n%d\n", memory_array[intermediate_table[i]->parameters[0]]);
	    	break;  // PRINT Instruction //

			case 17: 
                printf("\n%s\n", intermediate_table[i]->string);
	    	break;  // PRINTS Instruction //

			case 18:
				create_file(user->read_access, user->write_access, user->edit_access, 
				intermediate_table[i]->string_params[0], ATA_find_empty_sector(FILES_SECTOR_OFFSET));
			break;

			case 19:
				if (file_exist(intermediate_table[i]->string) == 1)
					if (user->edit_access <= find_file(intermediate_table[i]->string)->edit_level)
						delete_file(intermediate_table[i]->string);
			break;

			case 20:
				struct File* wfile = find_file(intermediate_table[i]->string_params[0]);
				if (wfile != NULL)
					if (user->write_access <= wfile->write_level)
						write_file(wfile, intermediate_table[i]->string_params[1]);
			break;

			case 21:
				struct File* rfile = find_file(intermediate_table[i]->string);
				if (rfile != NULL)
					if (user->read_access <= rfile->read_level) {
						int data = atoi(read_file(rfile));
						memory_array[intermediate_table[i]->parameters[0]] = data; 
					}
			break;

			case 7: 
                if (check_condition(memory_array[intermediate_table[i]->parameters[0]], memory_array[intermediate_table[i]->parameters[1]],
                    intermediate_table[i]->parameters[2]) == 0) {
                    i = intermediate_table[i]->parameters[3] - 1;  // IF Instruction //
                    continue;
                }
            break;

			case 6:  
                i = intermediate_table[i]->parameters[0] - 1; // JUMP or ELSE Instruction //
                continue;
            break;
		}

		i++;
	}
	return;
}
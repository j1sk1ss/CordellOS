#include "asm_compiler.h"


static int intermediate_index = 0;
static inlang** intermediate_table = NULL;

static int symbol_index = 0;
static symtable** symbol_tab = NULL;

static int blocks_index = 0;
static blctable** block_tab = NULL;


int main(int args, char* argv[]) {
	if (args <= 0) return -1;

	char* file_data = fread(argv[0]);
	char* file_pointer = file_data;

	intermediate_index = 0;
	symbol_index	   = 0;
	blocks_index       = 0;

	int stack[STACK_SIZE], top = -1;
	int memory_array[MEMORY_SIZE] = { 0 };
	int memory_index = VARIABLE_MEMORY_START - 1 ;  // 0 to 7 are already reserved

	symbol_tab = (symtable**)malloc(sizeof(symtable*) * 25);
	intermediate_table = (inlang**)malloc(sizeof(inlang*) * 50);
	block_tab = (blctable**)malloc(sizeof(blctable*) * 50);
	for (int i = 0; i < 25; i++) symbol_tab[i] = (symtable*)malloc(sizeof(symtable));
	for (int i = 0; i < 50; i++) intermediate_table[i] = (inlang*)malloc(sizeof(inlang));
	for (int i = 0; i < 50; i++) block_tab[i] = (blctable*)malloc(sizeof(blctable));

    // Determine the number of lines (count the newline characters)
    int num_lines = 0;
    char* newline_pos = file_data;
    while (*newline_pos) {
        if (*newline_pos == '\n') 
            num_lines++;
        
        newline_pos++;
    }

    // Allocate an array of char* to store the lines
    char** lines 	= (char**)malloc(num_lines * sizeof(char*));
    char* raw_line 	= strtok(file_data, "\n");
    int line_index 	= 0;

    while (raw_line != NULL) {
        lines[line_index] = (char*)malloc(strlen(raw_line) + 2);

        strcat(lines[line_index], raw_line);
		strcat(lines[line_index], "\n");
    	strcat(lines[line_index], "\0");

        line_index++;
        raw_line = strtok(NULL, "\n");
    }

	char tokens[10][10];
	char* buffer = (char*)malloc(10 * sizeof(char));
	int index = 0;

	// before start we save variable list //
	while (1) {
		if (strcmp(lines[index], "start-\n\0") == 0) 
			break;
		
		int row = 0, buffer_index = 0;

		// generating tokens //
		for (int i = 0; lines[index][i] != '\0'; i++) {
			if (lines[index][i] == ' ' || lines[index][i] == '\n') {
				buffer[buffer_index] = '\0';
				buffer_index = 0;

				strcpy(tokens[row++], buffer);
				free(buffer);
                
				buffer = (char*)malloc(10 * sizeof(char));
			}
			else buffer[buffer_index++] = lines[index][i];
		}

		// generating tokens //
		if (strcmp(tokens[0], "data") == 0) _data_func(tokens, memory_array, &memory_index);
		else if (strcmp(tokens[0], "const") == 0) _const_func(tokens, memory_array, &memory_index);
		if (++index > 11) return -1;
	}
	// before start //

	// after start//
	char instruction[INSTRUCTION_LENGTH], param[PARAMETERS_LENGTH];
	int opcode = -1, instruction_number = 0;

	while (index <= num_lines) {
		instruction_number++;
		if (lines[index][strlen(lines[index]) - 2] == '-'){
			lines[index][strlen(lines[index]) - 2] = '\0';
			block_tab[blocks_index]->instr_no = instruction_number--;
			strcpy(block_tab[blocks_index++]->name, lines[index++]);

			continue;
		}

		int i = 0, j = 0;
		while (lines[index][i] != ' ' && lines[index][i] != '\0' && lines[index][i] != '\n' && j < sizeof(instruction) - 1 ) 
			instruction[j++] = lines[index][i++];
		instruction[j] = '\0';

		while (lines[index][i] == ' ') i++;
		
		j = 0;
		while (lines[index][i] != '\0' && j < sizeof(param) - 1 && lines[index][i] != '\n') param[j++] = lines[index][i++];
		param[j] = '\0';
		
		opcode = _generate_operation_code(instruction);
		switch (opcode) {
		    case MOV_INSTRUCTION: 
				_mov_func(param, instruction_number);
			break;

			case ADD_INSTRUCTION:
				_bianryOperations_func(opcode, param, instruction_number);
			break;

			case SUB_INSTRUCTION:
				_bianryOperations_func(opcode, param, instruction_number);
			break;

			case MUL_INSTRUCTION:
				_bianryOperations_func(opcode, param, instruction_number);
			break;

			case JMP_INSTRUCTION: 
				if (strcmp(instruction, "else") == 0) _else_func(instruction_number, stack, &top);
				else _jump_func(param, instruction_number);
			break;

			case IF_INSTRUCTION:
				_if_func(param, instruction_number, stack, &top);
			break;

			case PRINT_INSTRUCTION:
				_print_func(param, instruction_number);
			break;

			case READ_INSTRUCTION:
				_read_func(param, instruction_number);
			break;

			case EIF_INSTRUCTION:
				_endif_func(instruction_number, stack, &top);
				instruction_number--;
			break;

			case END_INSTRUCTION:
				goto ending;

			case PRINTS_INSTRUCTION:
				_prints_func(param, instruction_number);
			break;

			case MKFILE_INSTRUCTION:
				_mkfile_func(param, instruction_number);
			break;

			case RMFILE_INSTRUCTION:
				_rmfile_func(param, instruction_number);
			break;

			case WFILE_INSTRUCTION:
				_wfile_func(param, instruction_number);
			break;

			case RFILE_INSTRUCTION:
				_rfile_func(param, instruction_number);
			break;

			case FOR_INSTRUCTION:
				_for_func(param, instruction_number, stack, &top);
			break;

			case EFOR_INSTRUCTION:
				_endfor_func(instruction_number, stack, &top);
				instruction_number--;
			break;

			case WHILE_INSTRUCTION:
				_while_func(param, instruction_number, stack, &top);
			break;

			case EWHILE_INSTRUCTION:
				_endwhile_func(instruction_number, stack, &top);
				instruction_number--;
			break;

			case PRINTL_INSTRUCTION:
				_printl_func(param, instruction_number);
			break;

			case CLEAR_INSTRUCTION:
				_clear_func(instruction_number);
			break;
		}

        index++;
	}
ending: 

	_asm_executor(memory_array, memory_index, 0, intermediate_index);
	
	for (int i = 0; i < 25; i++) free(symbol_tab[i]);
	for (int i = 0; i < 50; i++) free(intermediate_table[i]);
	for (int i = 0; i < 50; i++) free(block_tab[i]);
	
	free(symbol_tab);
	free(intermediate_table);
	free(block_tab);
	free(file_pointer);

	return 0;
}

void _const_func(char(*tokens)[10], int *memory, int *memory_index) {
	if (symbol_index == 0){
		symbol_tab[symbol_index]->address = VARIABLE_MEMORY_START;
		strcpy(symbol_tab[symbol_index]->variable_name, tokens[1]);
		symbol_tab[symbol_index]->size = CONSTANT_SIZE;
	}
	else {
		strcpy(symbol_tab[symbol_index]->variable_name, tokens[1]);
		symbol_tab[symbol_index]->size = CONSTANT_SIZE;
		if (symbol_tab[symbol_index - 1]->size != 0) symbol_tab[symbol_index]->address = symbol_tab[symbol_index - 1]->address + symbol_tab[symbol_index - 1]->size;
		else symbol_tab[symbol_index]->address = symbol_tab[symbol_index - 1]->address + 1;
	}

	symbol_index++;
	memory[*memory_index] = atoi(tokens[3]);
	(*memory_index)++;
}

void _data_func(char (*tokens)[10], int *memory, int *memory_index) {
	int i = 0, size = 0;
	char variable_name[VARIABLE_LENGTH];
    
	if (symbol_index == 0) {
		symbol_tab[symbol_index]->address = VARIABLE_MEMORY_START;
		while (tokens[1][i] != '\0' && tokens[1][i]!='['){
			variable_name[i] = tokens[1][i];
			i++;
		}

		variable_name[i] = '\0';
		strcpy(symbol_tab[symbol_index]->variable_name,variable_name); // entering variable name

		if (tokens[1][i] == '[') {
			while (tokens[1][i] != ']')
				size = size * 10 + tokens[1][i] - '0';

			i++;
		}

		if (size == 0) symbol_tab[symbol_index]->size = 1;
		else symbol_tab[symbol_index]->size = size;

		*memory_index = symbol_tab[symbol_index]->address + symbol_tab[symbol_index]->size;
		symbol_index++;

		return;
	}
	else {
		while (tokens[1][i] != '\0' && tokens[1][i] != '[') {
			variable_name[i] = tokens[1][i];
			i++;
		}

		variable_name[i] = '\0';
		strcpy(symbol_tab[symbol_index]->variable_name, variable_name); // entering variable name

		if (tokens[1][i] == '[') {
			i++;
			while (tokens[1][i] != ']') {
				size = size * 10 + tokens[1][i] - '0';
				i++;
			}
		}

		if (size == 0) symbol_tab[symbol_index]->size = 1;
		else symbol_tab[symbol_index]->size = size;
	
		if (symbol_tab[symbol_index - 1]->size != 0) symbol_tab[symbol_index]->address = symbol_tab[symbol_index - 1]->address + symbol_tab[symbol_index - 1]->size;
		else symbol_tab[symbol_index]->address = symbol_tab[symbol_index - 1]->address + 1;

		*memory_index = symbol_tab[symbol_index]->address + symbol_tab[symbol_index]->size;
		symbol_index++;
		return;
	}
}

int _generate_operation_code(char *instruction) {
	if (strcmp(instruction, "mov") == 0) 										return MOV_INSTRUCTION;
	if (strcmp(instruction, "add") == 0) 										return ADD_INSTRUCTION;
	if (strcmp(instruction, "sub") == 0) 										return SUB_INSTRUCTION;
	if (strcmp(instruction, "mul") == 0) 										return MUL_INSTRUCTION;
	if (strcmp(instruction, "jump") == 0 || strcmp(instruction, "else") == 0) 	return JMP_INSTRUCTION;
	if (strcmp(instruction, "if") == 0) 										return IF_INSTRUCTION;
	if (strcmp(instruction, "endif") == 0) 										return EIF_INSTRUCTION;
	if (strcmp(instruction, "eq") == 0) 										return EQ_INSTRUCTION;
	if (strcmp(instruction, "lt") == 0) 										return LT_INSTRUCTION;
	if (strcmp(instruction, "gt") == 0) 										return GT_INSTRUCTION;
	if (strcmp(instruction, "lteq") == 0) 										return LTEQ_INSTRUCTION;
	if (strcmp(instruction, "gteq") == 0) 										return GTEQ_INSTRUCTION;
	if (strcmp(instruction, "neq") == 0)										return NEQ_INSTRUCTION;
	if (strcmp(instruction, "clear") == 0)										return CLEAR_INSTRUCTION;
	if (strcmp(instruction, "prints") == 0) 									return PRINTS_INSTRUCTION;
	if (strcmp(instruction, "print") == 0) 										return PRINT_INSTRUCTION;
	if (strcmp(instruction, "printl") == 0)										return PRINTL_INSTRUCTION;
	if (strcmp(instruction, "read") == 0) 										return READ_INSTRUCTION;
	if (strcmp(instruction, "end") == 0) 										return END_INSTRUCTION;
	if (strcmp(instruction, "mkfile") == 0) 									return MKFILE_INSTRUCTION;
	if (strcmp(instruction, "rmfile") == 0) 									return RMFILE_INSTRUCTION;
	if (strcmp(instruction, "wfile") == 0) 										return WFILE_INSTRUCTION;
	if (strcmp(instruction, "rfile") == 0) 										return RFILE_INSTRUCTION;
	if (strcmp(instruction, "for") == 0)										return FOR_INSTRUCTION;
	if (strcmp(instruction, "endfor") == 0)										return EFOR_INSTRUCTION;
	if (strcmp(instruction, "while") == 0)										return WHILE_INSTRUCTION;
	if (strcmp(instruction, "endwhile") == 0)									return EWHILE_INSTRUCTION;
	return 1;
}

int _get_address(char* variable_name) {
	int i = 0, is_array = 0, array_index = 0;
	char temp[VARIABLE_LENGTH] = { 0 };

	if (variable_name[1] == 'x' && variable_name[0] >= 'a' && variable_name[0] <= 'h')
		return variable_name[0] - 'a';
	else {
		while (variable_name[i] != '\0') {
			if (variable_name[i] == '[') { // array

				int i = 0;
				while (variable_name[i] != '\0' && variable_name[i] != '[') 
					temp[i] = variable_name[i++];

				temp[i] = '\0';
				
				is_array = 1;
				i++;
				while (variable_name[i] != ']')
					array_index = array_index * 10 + variable_name[i++] - '0';
				
				break;
			}
            
			i++;
		}

		if (is_array == 0){
			int i = 0;
			while (variable_name[i] != '\0' && variable_name[i] != '\n') {
				temp[i] = variable_name[i];
				i++;
			}
			
			temp[i] = '\0'; 
		}
	}

	strcpy(variable_name, temp);
	for (int i = 0; i < symbol_index; i++) {
		if (strcmp(symbol_tab[i]->variable_name, variable_name) == 0) {
			if (is_array) return symbol_tab[i]->address + array_index;
			else return symbol_tab[i]->address;
		}
	}

	return -1;
}

void _mov_func(char *param, int instruction_number) {
	char dest[VARIABLE_LENGTH], src[VARIABLE_LENGTH];
	char *token;

	token = strtok(param, ", ");
	strcpy(dest, token);

	token = strtok(NULL, ", ");
	strcpy(src, token);

	intermediate_table[intermediate_index]->instruction_position = instruction_number;
	if (dest[1] == 'x' && dest[0] >= 'a' && dest[0] <= 'h') { // destination is register
		intermediate_table[intermediate_index]->opcode 			= 2;
		intermediate_table[intermediate_index]->parameters[0] 	= _get_address(dest);
		intermediate_table[intermediate_index]->parameters[1] 	= _get_address(src);
		intermediate_table[intermediate_index]->parameters[2] 	= -1;  // to run the for loop
	}
	else {
		intermediate_table[intermediate_index]->opcode 			= MOV_INSTRUCTION;   // destination is memory // i.e register to memory
		intermediate_table[intermediate_index]->parameters[0] 	= _get_address(dest);
		intermediate_table[intermediate_index]->parameters[1] 	= _get_address(src);
		intermediate_table[intermediate_index]->parameters[2] 	= -1;  // to run the for loop
	}

	intermediate_index++;
	return;
}

void _bianryOperations_func(int opcode, char *param, int instruction_number) {
	char dest[VARIABLE_LENGTH], operand1[VARIABLE_LENGTH], operand2[VARIABLE_LENGTH];
	/* get the first token */
	char *token;

	token = strtok(param, ", ");
	strcpy(dest, token);

	token = strtok(NULL, ", ");
	strcpy(operand1, token);

	token = strtok(NULL, ", ");
	strcpy(operand2, token);

	intermediate_table[intermediate_index]->opcode 					= opcode;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(dest);
	intermediate_table[intermediate_index]->parameters[1] 			= _get_address(operand1);
	intermediate_table[intermediate_index]->parameters[2] 			= _get_address(operand2);
	intermediate_table[intermediate_index]->parameters[3] 			= -1;

	intermediate_index++;
	return;
}

void _read_func(char *param, int instruction_number) {
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(param);
	intermediate_table[intermediate_index]->parameters[1] 			= -1;
	intermediate_table[intermediate_index]->opcode 					= READ_INSTRUCTION;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;

	intermediate_index++;
	return;
}

void _print_func(char *param, int instruction_number) {
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(param);
	intermediate_table[intermediate_index]->parameters[1] 			= -1;
	intermediate_table[intermediate_index]->opcode 					= PRINT_INSTRUCTION;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;

	intermediate_index++;
	return;
}

void _prints_func(char *param, int instruction_number) {
	intermediate_table[intermediate_index]->string = (char*)malloc(strlen(param));
	strcpy(intermediate_table[intermediate_index]->string, param);

	intermediate_table[intermediate_index]->opcode = PRINTS_INSTRUCTION;
	intermediate_table[intermediate_index++]->instruction_position = instruction_number;
	return;
}

void _printl_func(char *param, int instruction_number) {
	intermediate_table[intermediate_index]->string = (char*)malloc(strlen(param));
	strcpy(intermediate_table[intermediate_index]->string, param);

	intermediate_table[intermediate_index]->opcode = PRINTL_INSTRUCTION;
	intermediate_table[intermediate_index++]->instruction_position = instruction_number;
	return;
}

void _clear_func(int instruction_number) {
	intermediate_table[intermediate_index]->opcode = CLEAR_INSTRUCTION;	
	intermediate_table[intermediate_index++]->instruction_position = instruction_number;
	return;
}

void _mkfile_func(char* param, int instruction_number) {
	intermediate_table[intermediate_index]->string_params[0] = (char*)malloc(strlen(param));
	strcpy(intermediate_table[intermediate_index]->string_params[0], param);

	intermediate_table[intermediate_index]->opcode 					= MKFILE_INSTRUCTION;
	intermediate_table[intermediate_index++]->instruction_position 	= instruction_number;
	return;
}

void _rmfile_func(char* param, int instruction_number) {
	intermediate_table[intermediate_index]->string = (char*)malloc(strlen(param));
	strcpy(intermediate_table[intermediate_index]->string, param);

	intermediate_table[intermediate_index]->opcode 					= RMFILE_INSTRUCTION;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;

	intermediate_index++;
	return;
}

void _wfile_func(char* param, int instruction_number) {
	char* first = strtok(param, " ");
	intermediate_table[intermediate_index]->string_params[0] = (char*)malloc(strlen(first));
	strcpy(intermediate_table[intermediate_index]->string_params[0], first);

	first = strtok(NULL, " ");
	intermediate_table[intermediate_index]->string_params[1] = (char*)malloc(strlen(first));
	strcpy(intermediate_table[intermediate_index]->string_params[1], first);

	intermediate_table[intermediate_index]->opcode 					= WFILE_INSTRUCTION;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;

	intermediate_index++;
	return;
}

void _rfile_func(char *param, int instruction_number) {
	char* first = strtok(param, " ");
	intermediate_table[intermediate_index]->parameters[0] = _get_address(first);

	first = strtok(NULL, " ");
	intermediate_table[intermediate_index]->string = (char*)malloc(strlen(first));
	strcpy(intermediate_table[intermediate_index]->string, first);

	intermediate_table[intermediate_index]->opcode 					= RFILE_INSTRUCTION;
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;

	intermediate_index++;
	return;
}

void _for_func(char *param, int instruction_number, int *stack, int *top) {
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;
	intermediate_table[intermediate_index]->opcode 					= FOR_INSTRUCTION;
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(param);
	intermediate_table[intermediate_index]->parameters[3] 			= -2;
	intermediate_table[intermediate_index]->parameters[4] 			= -1;

	stack[++(*top)] = instruction_number;
	intermediate_index++;
	return;
}

void _endfor_func(int instruction_number, int *stack, int *top) {
	int poped_value = stack[(*top)--];
	int i = intermediate_index;
	
	while (intermediate_table[i--]->instruction_position != poped_value);
	i++;
	intermediate_table[i]->parameters[3] = instruction_number;

	return;
}

void _while_func(char *param, int instruction_number, int *stack, int *top) {
	char operand1[VARIABLE_LENGTH] = { 0 }, oper[4] = { 0 }, operand2[VARIABLE_LENGTH] = { 0 };

	int i = 0;
    int j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(operand1) - 1) operand1[j++] = param[i++];
    operand1[j] = '\0';

    while (param[i] == ' ') i++;
    j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(oper) - 1) oper[j++] = param[i++];
    oper[j] = '\0';

    while (param[i] == ' ') i++;
    j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(operand2) - 1) operand2[j++] = param[i++];
    operand2[j] = '\0';

	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;
	intermediate_table[intermediate_index]->opcode 					= WHILE_INSTRUCTION;
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(operand1);
	intermediate_table[intermediate_index]->parameters[1] 			= _get_address(operand2);
	intermediate_table[intermediate_index]->parameters[2] 			= _generate_operation_code(oper);
	intermediate_table[intermediate_index]->parameters[3] 			= -2;
	intermediate_table[intermediate_index]->parameters[4] 			= -1;

	stack[++(*top)] = instruction_number;
	intermediate_index++;
	return;
}

void _endwhile_func(int instruction_number, int *stack, int *top) {
	int poped_value = stack[(*top)--];
	int i = intermediate_index;
	
	while (intermediate_table[i--]->instruction_position != poped_value);
	i++;
	intermediate_table[i]->parameters[3] = instruction_number;

	return;
}

void _if_func(char *param, int instruction_number, int *stack, int *top) {
	char operand1[VARIABLE_LENGTH] = { 0 }, oper[4] = { 0 }, operand2[VARIABLE_LENGTH] = { 0 };
	int i = 0;
    int j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(operand1) - 1) operand1[j++] = param[i++];
    operand1[j] = '\0';

    while (param[i] == ' ') i++;
    j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(oper) - 1) oper[j++] = param[i++];
    oper[j] = '\0';

    while (param[i] == ' ') i++;
    j = 0;

    while (param[i] != ' ' && param[i] != '\0' && j < sizeof(operand2) - 1) operand2[j++] = param[i++];
    operand2[j] = '\0';

	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;
	intermediate_table[intermediate_index]->opcode 					= IF_INSTRUCTION;
	intermediate_table[intermediate_index]->parameters[0] 			= _get_address(operand1);
	intermediate_table[intermediate_index]->parameters[1] 			= _get_address(operand2);
	intermediate_table[intermediate_index]->parameters[2] 			= _generate_operation_code(oper);
	intermediate_table[intermediate_index]->parameters[3] 			= -2;
	intermediate_table[intermediate_index]->parameters[4] 			= -1;
	
	stack[++(*top)] = instruction_number;
	intermediate_index++;
	return;
}

void _else_func(int instruction_number, int *stack, int *top) {
	intermediate_table[intermediate_index]->instruction_position 	= instruction_number;
	intermediate_table[intermediate_index]->opcode 					= ELS_INSTRUCTION;
	intermediate_table[intermediate_index]->parameters[0] 			= -2;
	intermediate_table[intermediate_index]->parameters[1] 			= -1;
	// push into stack //
	stack[++(*top)] = instruction_number;
	// push into stack //
	intermediate_index++;
	return;
}

void _endif_func(int instruction_number, int *stack, int *top) {
	int poped_value = stack[(*top)--];
	int i = intermediate_index;
	
	while (intermediate_table[i--]->instruction_position != poped_value);
	i++;
	intermediate_table[i]->parameters[0] = instruction_number;

	int temp = poped_value;
	poped_value = stack[(*top)--];

	while (intermediate_table[i--]->instruction_position != poped_value);
	i++;
	intermediate_table[i]->parameters[3] = temp + 1;

	return;
}

void _jump_func(char *param, int instruction_number) {
	intermediate_table[intermediate_index]->instruction_position = instruction_number;
	intermediate_table[intermediate_index]->opcode = JMP_INSTRUCTION;

	for (int i = 0; i < blocks_index; i++)
		if (strcmp(block_tab[i]->name, param) == 0){
			intermediate_table[intermediate_index]->parameters[0] = block_tab[i]->instr_no;
			intermediate_table[intermediate_index]->parameters[1] = -1;
			break;
		}

	intermediate_index++;
	return;
}

void display_symbol_table() {
	printf("\n---------------Symbol Table is ----------\n");
	for (int i = 0; i < symbol_index; i++)
		printf("\nvariable name is %s,address is %d, size is %d", symbol_tab[i]->variable_name, symbol_tab[i]->address, symbol_tab[i]->size);

	return;
}

void display_intermediate_table() {
	printf("\n---------------Instruction Table is ----------\n");
	for (int i = 0; i < intermediate_index; i++){
		printf("\n%d : %d : ", intermediate_table[i]->instruction_position, intermediate_table[i]->opcode);
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

int _check_condition(int operand1, int operand2, int opcode) {
	switch(opcode){
		case EQ_INSTRUCTION: 
			if (operand1 == operand2)
				return 1;
		break;

		case LT_INSTRUCTION:
			if (operand1 < operand2)
				return 1;
		break;

		case GT_INSTRUCTION: 
			if (operand1 > operand2)
				return 1;
		break;

		case LTEQ_INSTRUCTION: 
			if (operand1 <= operand2)
				return 1;
		break;

		case GTEQ_INSTRUCTION: 
			if (operand1 >= operand2)
				return 1;
		break;

		case NEQ_INSTRUCTION: 
			if (operand1 != operand2)
				return 1;
		break;
	}
	
	return 0;
}

void _asm_executor(int *memory_array, int memory_index, int start, int end) {
	for (int i = start; i < end;) {  // iterating on the intermediate language table
		switch (intermediate_table[i]->opcode) {
			case READ_INSTRUCTION:  
				printf("\n");
				char input_buffer[128] = { '\0' };
				char stp[2] = { '\n', '\0' };
				keyboard_read(VISIBLE_KEYBOARD, -1, stp, input_buffer);

                memory_array[intermediate_table[i]->parameters[0]] = atoi(input_buffer);
			break; // READ Instruction //

			case MOV_INSTRUCTION: 
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]]; 
		    break; // MOVE Instruction //

			case ADD_INSTRUCTION:
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]] +
																		 memory_array[intermediate_table[i]->parameters[2]];
			break; // ADD Instruction //

			case SUB_INSTRUCTION:
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]] -
																		 memory_array[intermediate_table[i]->parameters[2]];
		    break; // SUB Instruction //

			case MUL_INSTRUCTION:
                memory_array[intermediate_table[i]->parameters[0]] = memory_array[intermediate_table[i]->parameters[1]] *
																		 memory_array[intermediate_table[i]->parameters[2]];
		    break; // MUL Instruction //

			case PRINT_INSTRUCTION: 
                printf("\n%d", memory_array[intermediate_table[i]->parameters[0]]);
	    	break;  // PRINT Instruction //

			case PRINTS_INSTRUCTION: 
                printf("%s", intermediate_table[i]->string);
	    	break;  // PRINTS Instruction //

			case PRINTL_INSTRUCTION:
                printf("\n%s", intermediate_table[i]->string);
	    	break;  // PRINTL Instruction //

			case MKFILE_INSTRUCTION:
				mkfile(intermediate_table[i]->string_params[1], intermediate_table[i]->string_params[0]);
			break;

			case RMFILE_INSTRUCTION:
				if (cexists(intermediate_table[i]->string) == 1) rmcontent(intermediate_table[i]->string_params[1]);
			break;

			case WFILE_INSTRUCTION:
				if (cexists(intermediate_table[i]->string_params[0]) == 1) fwrite(intermediate_table[i]->string_params[0], intermediate_table[i]->string_params[1]);
			break;

			case RFILE_INSTRUCTION:
				if (cexists(intermediate_table[i]->string_params[0]) == 1) {
					char* fat_data = fread(intermediate_table[i]->string_params[0]);
					int data = atoi(fat_data);
					free(fat_data);

					memory_array[intermediate_table[i]->parameters[0]] = data;
				}
			break;

			case IF_INSTRUCTION: 
                if (_check_condition(memory_array[intermediate_table[i]->parameters[0]], memory_array[intermediate_table[i]->parameters[1]],
                    intermediate_table[i]->parameters[2]) == 0) {
                    i = intermediate_table[i]->parameters[3] - 1;  // IF Instruction //
                    continue;
                }
            break;

			case FOR_INSTRUCTION:
				for (int j = 0; j < memory_array[intermediate_table[i]->parameters[0]]; j++) 
					_asm_executor(memory_array, memory_index, i + 1, intermediate_table[i]->parameters[3] - 1);

				i = intermediate_table[i]->parameters[3] - 1;
			break;

			case WHILE_INSTRUCTION:
				while (_check_condition(memory_array[intermediate_table[i]->parameters[0]], memory_array[intermediate_table[i]->parameters[1]],
                    intermediate_table[i]->parameters[2]) != 0) 
					_asm_executor(memory_array, memory_index, i + 1, intermediate_table[i]->parameters[3] - 1);

				i = intermediate_table[i]->parameters[3] - 1;
			break;

			case CLEAR_INSTRUCTION:
				clrscr();
			break;

			case JMP_INSTRUCTION:  
                i = intermediate_table[i]->parameters[0] - 1; // JUMP or ELSE Instruction //
                continue;
            break;
		}

		i++;
	}
	
	return;
}
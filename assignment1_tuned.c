#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _CRT_SECURE_NO_WARNINGS //sscanf 에러 무시 실행.
#define MEMORY_START 1000 // pc 시작 값 1000 <- 명세서 참고.

// opcode per type
#define R_TYPE      0b0110011 // ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
#define I_TYPE      0b0010011 // ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI
#define I_TYPE_LW   0b0000011 // LW
#define I_TYPE_JALR 0b1100111 // JALR
#define SB_TYPE     0b1100011 // BEQ, BNE, BGE, BLT
#define UJ_TYPE     0b1101111 // JAL
#define S_TYPE      0b0100011 // SW


typedef struct {
    char* name;
    int address;
} Label;

void operate_instruction(char* instruction, FILE* o_file, int* pc, Label* labels, int label_count);
int parse_instruction(char* line, FILE* o_file, FILE* trace_file, int* pc, Label* labels, int label_count);

int is_label(char* line);
void add_label(char* line, Label* labels, int* label_count, int* pc);
Label find_label(char* label_name, Label* labels, int label_count);

int label_count = 0;        //라벨 개수

int main(int argc, char* argv[]) {

    char instruction[50];       //명령어
    char* label;                //라벨
    char input_name[32];        //입력받는 문자열 
    char* file_name;            //파일 이름
    char o_filename[32];        //.o 파일 이름  <- 명령어 2진수 값들
    char trace_filename[32];    //.trace 파일 이름  <- pc 값들 
    char line[128];

    Label labels[100];          //라벨들을 저장할 배열

    int pc = MEMORY_START;
    FILE* input_file;
    FILE* o_file;
    FILE* trace_file;

    while (1) {
        printf(">> Enter Input File Name : ");
        scanf("%s", input_name);

        if (strcmp(input_name, "terminate") == 0)
            break;
        //입력받은 이름이 terminate이면 루프 종료.

        input_file = fopen(input_name, "r");

        //입력받은 값의 이름을 가진 파일을 읽어들여오고("r"ead),
        if (!input_file) {
            printf("Input file does not exist!!\n");
            continue;
        }

        //이름이 틀리면, 다음 항목 출력 후 루프 다시 진행.
        char eraser[] = ".";
        file_name = strtok(input_name, eraser);

        snprintf(o_filename, sizeof(o_filename), "%s.o", file_name);
        snprintf(trace_filename, sizeof(trace_filename), "%s.trace", file_name);
        //각각 .o, .trace 확장자형태를 포함한 파일이름 지정.

        o_file = fopen(o_filename, "w");
        trace_file = fopen(trace_filename, "w");
        //해당 파일들을 열어서 적겠다. "w"rite

        for (int pass = 1 ; pass <= 2; pass++){
            while (fgets(line, sizeof(line), input_file)) {
                if (line[0] == '\n') continue;  //공백 라인 무시
                char* clean_line = strtok(line, "\n");
                printf("%s\n", clean_line);
                if (pass == 1) { // 라벨 저장 단계
                    if (is_label(clean_line)) {
                        add_label(clean_line, labels, &label_count, &pc);
                    }
                }
                else { // 실제 수행 단계
                    if (!parse_instruction(clean_line, o_file, trace_file, &pc, labels, label_count)) {
                        printf("Syntax Error!!\n");
                        fclose(o_file);
                        fclose(trace_file);
                        remove(o_filename);
                        remove(trace_filename);
                        break;
                    }
                }
            }
            fseek(input_file, 0, SEEK_SET);
        }
        fclose(input_file);
        fclose(o_file);
        fclose(trace_file);
    }
    return 0;
}

void write_trace(FILE* trace_file, int pc) {
    fprintf(trace_file, "%d\n", pc);
}
// PC 값을 .trace 파일에 기록하는 함수.

int parse_instruction(char* line, FILE* o_file, FILE* trace_file, int* pc, Label* labels, int label_count) {
    write_trace(trace_file, *pc);
    operate_instruction(line, o_file, pc, labels, label_count);
    *pc += 4;   // 다음 명령어로 PC 증가
    return 1;   // 정상 처리
}


//10진수를 2진수로 변환해주는 함수
void print_binary(unsigned int num, FILE* o_file) {
    for (int i = 31; i >= 0; i--) {
        fprintf(o_file, "%d", (num >> i) & 1);
    }
    fprintf(o_file, "\n");
}

unsigned int get_opcode(char* opcode){
    char* r_type[] = { "ADD", "SUB", "AND", "OR", "XOR", "SLL", "SRL", "SRA" };
    char* i_type[] = { "ADDI", "ANDI", "ORI", "XORI", "SLLI", "SRLI", "SRAI", "LW", "JALR" };
    char* sb_type[] = { "BEQ", "BNE", "BGE", "BLT" };
    char* uj_type[] = { "JAL" };
    char* s_type[] = { "SW" };

    // r_type?
    for (int i = 0; i < 8; i++) {
        if (!strcmp(opcode, r_type[i])) return R_TYPE;
    }
    // i_type?
    for (int i = 0; i < 9; i++) {
        if (!strcmp(opcode, "LW"))      return I_TYPE_LW;
        if (!strcmp(opcode, "JALR"))    return I_TYPE_JALR;
        if (!strcmp(opcode, i_type[i])) return I_TYPE;
    }
    // sb_type?
    for (int i = 0; i < 5; i++) {
        if (!strcmp(opcode, sb_type[i])) return SB_TYPE;
    }
    // uj_type?
    for (int i = 0; i < 2; i++) {
        if (!strcmp(opcode, uj_type[i])) return UJ_TYPE;
    }
    // s_type?
    for (int i = 0; i < 2; i++) {
        if (!strcmp(opcode, s_type[i])) return S_TYPE;
    }

    // nothing matched
    printf("\nSyntax Error!!\n");
    return 0;
}

// label 관련 함수
int is_label(char* line) {
    if (strchr(line, ':')) return 1;
    return 0;
}

void add_label(char* line, Label* labels, int* label_count, int* pc) {
    char* label_name = strtok(line, ":");
    labels[*label_count].name = label_name;
    labels[*label_count].address = *pc;
    (*label_count)++;
}

Label find_label(char* label_name, Label* labels, int label_count) {
    for (int i = 0; i < label_count; i++) {
        if (!strcmp(label_name, labels[i].name)) return labels[i];
    }
    Label empty_label = { NULL, -1 };
    return empty_label;
}

//opcode 구별해서 2진수로 변환 및 pc 업데이트
void operate_instruction(char* instruction, FILE* o_file, int* pc, Label* labels, int label_count) {
    printf("%s", instruction);
    char opcode[10];
    int offset, rd = 0, rs1 = 0, rs2 = 1;
    char label[10];
    unsigned int out_code_num = 0;

    sscanf(instruction, "%s", opcode);

    // check if label
    if (is_label(instruction)) {
        return;
    }

    // check if EXIT
    if (!strcmp(opcode, "EXIT")) {
        print_binary(0xffff, o_file);
        exit(1);
    }

    // syntax error
    if (get_opcode(opcode) == 0){
        printf("\nSyntax Error!!\n");
    }

    // 1. R type
    // ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
    else if (get_opcode(opcode) == R_TYPE) {

        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);
        out_code_num = R_TYPE;
        
        // funct7 - 7bit | rs2 - 5bit | rs1 - 5bit | fucnt3 - 3bit | rd - 5bit | opcode - 7bit
        if (!strcmp(opcode, "ADD"))         out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        else if (!strcmp(opcode, "SUB"))    out_code_num |= (0b0100000 << 25) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        else if (!strcmp(opcode, "AND"))    out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b111 << 12);
        else if (!strcmp(opcode, "OR"))     out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b110 << 12);
        else if (!strcmp(opcode, "XOR"))    out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b100 << 12);
        else if (!strcmp(opcode, "SLL"))    out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b001 << 12);
        else if (!strcmp(opcode, "SRL"))    out_code_num |= (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);
        else if (!strcmp(opcode, "SRA"))    out_code_num |= (0b0100000 << 25) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);
    }

    // 2. I type
    // ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, LW, JALR
    else if (get_opcode(opcode) == I_TYPE || get_opcode(opcode) == I_TYPE_LW || get_opcode(opcode) == I_TYPE_JALR) {

        if (!strcmp(opcode, "LW") || !strcmp(opcode, "JALR")){
            sscanf(instruction, "%*s x%d, %d(x%d)", &rd, &rs2, &rs1);

            if (!strcmp(opcode, "LW"))          out_code_num = I_TYPE_LW | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b010 << 12);
            else if (!strcmp(opcode, "JALR"))   out_code_num = I_TYPE_JALR | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        }
        else {
            sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);
            out_code_num = I_TYPE;

            // imm - 12bit | rs1 - 5bit | fucnt3 - 3bit | rd - 5bit | opcode - 7bit
            if (!strcmp(opcode, "ADDI"))        out_code_num |= ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
            else if (!strcmp(opcode, "ANDI"))   out_code_num |= ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b111 << 12);
            else if (!strcmp(opcode, "ORI"))    out_code_num |= ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b110 << 12);
            else if (!strcmp(opcode, "XORI"))   out_code_num |= ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b100 << 12);
            else if (!strcmp(opcode, "SLLI"))   out_code_num |= ((rs2 & 0x1FF) << 20) | (rs1 << 15) | (rd << 7) | (0b001 << 12);
            else if (!strcmp(opcode, "SRLI"))   out_code_num |= ((rs2 & 0x1FF) << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);
            else if (!strcmp(opcode, "SRAI"))   out_code_num |= (0b0100000 << 25) | ((rs2 & 0x1FF) << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);
        }
    }

    // 3. SB type
    // BEQ, BNE, BGE, BLT
    else if (get_opcode(opcode) == SB_TYPE) {
        // sscanf(instruction, "%*s x%d, x%d, %d", &rs1, &rs2, &rd);
        
        sscanf(instruction, "%*s x%d, x%d, %s", &rs1, &rs2, label);

        Label label_info = find_label(label, labels, label_count);
        rd = label_info.address - *pc;
        // rd = label_info.address;

        out_code_num = SB_TYPE | ((rd & 0x0001) << 25) | (((rd & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((rd & 0x01E) << 7) | ((rd & 0x800) << 7);

        // imm - 12bit | rs2 - 5bit | rs1 - 5bit | fucnt3 - 3bit | rd - 5bit | opcode - 7bit
        if (!strcmp(opcode, "BEQ"))        out_code_num |= (0b000 << 12);
        else if (!strcmp(opcode, "BNE"))   out_code_num |= (0b001 << 12);
        else if (!strcmp(opcode, "BGE"))   out_code_num |= (0b101 << 12);
        else if (!strcmp(opcode, "BLT"))   out_code_num |= (0b100 << 12);
    }
    
    // 4. UJ type
    // JAL
    else if (get_opcode(opcode) == UJ_TYPE) {
        //JAL x1, func1
        sscanf(instruction, "%*s x%d, %s", &rd, label);

        Label label_info = find_label(label, labels, label_count);
        offset = label_info.address - *pc;
        // offset = label_info.address;
        out_code_num = UJ_TYPE | ((offset & 0x0001) << 31) | (((offset & 0x7FE) >> 1) << 31) | (((offset & 0x800) >> 11) << 31) | (rd << 7) | (0b000 << 12);
    }

    // 5. S type
    // SW
    else if (get_opcode(opcode) == S_TYPE) {
        sscanf(instruction, "%*s x%d, %d(x%d)", &rs2, &offset, &rs1);
        out_code_num = S_TYPE | ((offset & 0xFFF) << 20) | (rs2 << 20) | (rs1 << 15) | (0b010 << 12);
    }

    print_binary(out_code_num, o_file);
}
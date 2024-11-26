#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma warning(disable:4996)
//sscanf 에러 무시 실행.
#define MEMORY_START 1000
// pc 시작 값 1000 <- 명세서 참고.

//라벨 명칭 + 라벨에 해당하는 pc주솟값은 세트 -> 구조체사용
    //struct Label 일일이 안해도 되고 -> Label로 선언가능.
struct Label {
    char* label_name;   //라벨 명칭
    int label_address;  //라벨 주솟값
};
typedef struct Label Label;

Label label_arr[256];     //구조체 Label들을 저장할 배열.
int label_count = 0;        //Label배열에 들어있는 개수

void op_num(char* instruction, FILE* o_file, int* pc, int scan_count);
void add_label(char* line, Label* labels, int* pc);
int parse_instruction(char* line, FILE* o_file, FILE* trace_file, int* pc, int scan_count);

void write_trace(FILE* trace_file, int pc) {
    fprintf(trace_file, "%d\n", pc);
}


int main(int argc, char* argv[]) {

    char instruction[50];       //명령어
    char input_name[32];        //입력받는 문자열 
    char* file_name;            //파일 이름
    char o_filename[32];        //.o 파일 이름  <- 명령어 2진수 값들
    char trace_filename[32];    //.trace 파일 이름  <- pc 값들 
    char line[128];             //한줄
    //배열 지정 - 지정만큼 크기할당 
    //포인터로 지정 -> 주소값 할당 
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

        for (int scan_count = 1; scan_count <= 2; scan_count++) {
            pc = MEMORY_START;
            while (fgets(line, sizeof(line), input_file)) {
                // if (line[0] == '\n') continue;  //공백 라인 무시
                // 공백 제거
                char *trimmed_line = strtok(line, "\n");
                if (!trimmed_line || trimmed_line[0] == '#') {
                    continue; // 빈 줄이나 주석은 건너뜀
                }
                // 레이블 처리
                if (strchr(trimmed_line, ':')) {
                    char *label = strtok(trimmed_line, ":");
                    if (scan_count == 1) {
                        add_label(label, label_arr, &pc);
                    }
                } else if (!parse_instruction(line, o_file, trace_file, &pc, scan_count)) {
                    printf("rrrSyntax Error!!\n");
                    fclose(o_file);
                    fclose(trace_file);
                    remove(o_filename);
                    remove(trace_filename);
                    break;
                }
                
                if(scan_count == 2) write_trace(trace_file, pc);
                pc += 4;   // 다음 명령어로 PC 증가
            }
            fseek(input_file, 0, SEEK_SET);
            //fgets 함수 끝난 시점에 포인터가 맨 뒤이므로 다시 맨 앞으로 돌려주는 함수.
        }
        fclose(input_file);
        fclose(o_file);
        fclose(trace_file);
        
        for (int i = 0; i < label_count; i++) {
            free(label_arr[i].label_name);
        }
    }
    return 0;
}



void add_label(char* line, Label* labels, int* pc) {
    // char* name = strtok(line, ":");   //:를 잘라내고 남은 것 라벨 이름으로 저장.

    // // labels[label_count].label_name = name;
    // labels[label_count].label_name = strdup(name);

    // labels[label_count].label_address = *pc;
    // (label_count)++;

    char* name = strtok(line, ":");  // ':'로 라벨 이름 분리
    if (name == NULL) {
        fprintf(stderr, "Error: Invalid label format\n");
        return;
    }

    labels[label_count].label_name = strdup(name);  // 메모리 복사
    labels[label_count].label_address = *pc;
    printf("%s %d\n", labels[label_count].label_name, labels[label_count].label_address);
    label_count++;

    if (label_count > 256) {
        fprintf(stderr, "Error: Label array overflow\n");
        exit(EXIT_FAILURE);  // 프로그램 강제 종료
    }
}

// PC 값을 .trace 파일에 기록하는 함수.

int parse_instruction(char* line, FILE* o_file, FILE* trace_file, int* pc, int scan_count) {
    if (scan_count == 2) {
        op_num(line, o_file, pc, scan_count);
        return 1;
    }

    return 1;   // 정상 처리
}


//10진수를 2진수로 변환해주는 함수
void print_binary(unsigned int num, FILE* o_file) {
    for (int i = 31; i >= 0; i--) {
        fprintf(o_file, "%d", (num >> i) & 1);
    }
    fprintf(o_file, "\n");
}

//opcode 구별해서 2진수로 뱉어내는 함수
void op_num(char* instruction, FILE* o_file, int* pc, int scan_count) {
    char opcode[10];
    int offset, rd = 0, rs1 = 0, rs2 = 1;
    char label[10]; 

    unsigned int r_opcode = 0b0110011;
    //아래는 똑같은 맥락으로 타입 별 코드 미리 선언.
    unsigned int i_opcode = 0b0010011;
    unsigned int sb_opcode = 0b1100011;

    sscanf(instruction, "%s", opcode);
    // instruction 에서 s부분잘라내서 opcode에 저장

    // register 관리
    
    

    //R-type 목록
    if (strcmp(opcode, "ADD") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);
        //s는 읽기만하고, 나머지는 각각 읽고 각 rd, rs1, rs2에 저장.

        // funct7 - 7bit | rs2 - 5bit | rs1 - 5bit | fucnt3 - 3bit | rd - 5bit | opcode - 7bit 

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        //타입에 해당하는 opcode에 해당하는 비트 먼저 넣고, 순서대로 왼쪽으로 밀어 넣기.
        //fprintf(o_file)
        //printf("ADD");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
        
    }
    else if (strcmp(opcode, "SUB") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (0b0100000 << 25) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        //0b0100000 funct7에 해당
        //printf("SUB");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "AND") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b111 << 12);
        
        //printf("AND");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "OR") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b110 << 12);
        
        //printf("OR");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "XOR") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b100 << 12);
        
        //printf("XOR");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "SLL") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b001 << 12);

        //printf("SLL");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "SRL") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);

        //printf("SRL");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "SRA") == 0) {
        sscanf(instruction, "%*s x%d, x%d, x%d", &rd, &rs1, &rs2);

        unsigned int out_code_num = r_opcode | (0b0100000 << 25) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);
        //0b0100000 funct7에 해당
        //printf("SRA");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    
    //I-type 목록
    //마스크기법으로 12비트 잘라내기/ 5비트 잘라내기
    else if (strcmp(opcode, "ADDI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);
        //i타입에서는 rs2가 imm역할 수행.

        unsigned int out_code_num = i_opcode | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);
        
        //printf("ADDI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "ANDI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b111 << 12);

        //printf("ANDI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "ORI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | ((rs2 & 0xFFF) << 20) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b110 << 12);

        //printf("ORI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    else if (strcmp(opcode, "XORI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | ((rs2 & 0xFFF) << 20) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b100 << 12);

        //printf("XORI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    //형식다름 - 5비트잘라내기
    else if (strcmp(opcode, "SLLI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | ((rs2 & 0x1FF) << 20) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b001 << 12);

        //printf("SLLI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    //형식다름 - 5비트 잘라내기
    else if (strcmp(opcode, "SRLI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | ((rs2 & 0x1FF) << 20) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);

        //printf("SRLI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    //형식다름 - 5비트 잘라내고 + 맨앞 0100000
    else if (strcmp(opcode, "SRAI") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rd, &rs1, &rs2);

        unsigned int out_code_num = i_opcode | (0b0100000 << 25 ) | ((rs2 & 0x1FF) << 20) | (rs2 << 20) | (rs1 << 15) | (rd << 7) | (0b101 << 12);

        //printf("SRAI");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    //형식다름
    else if (strcmp(opcode, "LW") == 0) {
        sscanf(instruction, "%*s x%d, %d(x%d)", &rd, &rs2, &rs1);
        //rs2  offset역할

        unsigned int out_code_num = 0b0000011 | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b010 << 12);

        //printf("LW");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    //형식다름
    else if (strcmp(opcode, "JALR") == 0) {
        sscanf(instruction, "%*s x%d, %d(x%d)", &rd, &rs2, &rs1);

        unsigned int out_code_num = 0b1100111 | ((rs2 & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | (0b000 << 12);

        //printf("JALR");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
    }
    
    //SB-type 목록
    else if (strcmp(opcode, "BEQ") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %s", &rs1, &rs2, label);
        char* cleaned_label = strtok(label, ":");
        

        for (int i = 0; i < label_count; i++) {
            if (strcmp(cleaned_label, label_arr[i].label_name) == 0) {
                unsigned int out_code_num = sb_opcode | ((label_arr[i].label_address & 0x0001) << 25) | (((label_arr[i].label_address & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((label_arr[i].label_address & 0x01E) << 7) | ((label_arr[i].label_address & 0x800) << 7) | (0b000 << 12);

                //printf("BEQ");    //테스트용 문구
                print_binary(out_code_num, o_file);   //출력 
                break;
            }
            
        }

    }
    else if (strcmp(opcode, "BNE") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %s", &rs1, &rs2, label);
        char* cleaned_label = strtok(label, ":");

        for (int i = 0; i < label_count; i++) {
            if (strcmp(cleaned_label, label_arr[i].label_name) == 0) {
                rd = label_arr[i].label_address - *pc;
                // unsigned int out_code_num = sb_opcode | ((label_arr[i].label_address & 0x0001) << 25) | (((label_arr[i].label_address & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((label_arr[i].label_address & 0x01E) << 7) | ((label_arr[i].label_address & 0x800) << 7) | (0b001 << 12);
                unsigned int out_code_num = sb_opcode | ((rd & 0x0001) << 25) | (((rd & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((rd & 0x01E) << 7) | ((rd & 0x800) << 7) | (0b001 << 12);
                //printf("BNE");    //테스트용 문구
                print_binary(out_code_num, o_file);   //출력 
                break;
            }
        }
       
    }
    else if (strcmp(opcode, "BGE") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %s", &rs1, &rs2, label);
        char* cleaned_label = strtok(label, ":");

        for (int i = 0; i < label_count; i++) {
            if (cleaned_label == NULL || label_arr[i].label_name == NULL) {
                continue;  // 비교를 건너뜁니다
            }
            if (strcmp(cleaned_label, label_arr[i].label_name) == 0) {
                rd = label_arr[i].label_address - *pc;
                printf("%d\n", label_arr[i].label_address);
                printf("%d\n", *pc);
                printf("%d\n", rd);
                // unsigned int out_code_num = sb_opcode | ((label_arr[i].label_address & 0x0001) << 25) | (((label_arr[i].label_address & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((label_arr[i].label_address & 0x01E) << 7) | ((label_arr[i].label_address & 0x800) << 7) | (0b101 << 12);
                // printf("BGE");    //테스트용 문구

                 // 12비트 부호 확장 처리
                int rd_signed = rd & 0xFFF; // 12비트로 제한
                if (rd & 0x800) { // 부호 비트가 1이면
                    rd_signed |= 0xFFFFF000; // 부호 확장
                }
                unsigned int out_code_num = sb_opcode | ((rd_signed & 0x1000) << 19)
                | (((rd_signed & 0x7E0) >> 5) << 25)
                | (rs2 << 20) | (rs1 << 15) | ((rd_signed & 0x01E) << 8) | ((rd_signed & 0x800) >> 4) | (0b101 << 12);

                print_binary(out_code_num, o_file);   //출력 
                
                // greated than? less than? equal?
                bool is_greater = false;
                // rs1, rs2 비교
                if (rs1 >= rs2) {
                    is_greater = true;
                }

                if (is_greater) {
                    // pc update
                    *pc = label_arr[i].label_address;
                }

                break;
            }
        }
    }
    else if (strcmp(opcode, "BLT") == 0) {
        sscanf(instruction, "%*s x%d, x%d, %d", &rs1, &rs2, &rd);
        //rd가 imm12 역할수행

        unsigned int out_code_num = sb_opcode | ((rd & 0x0001) << 25) | (((rd & 0x7E0) >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | ((rd & 0x01E) << 7) | ((rd & 0x800) << 7) | (0b100 << 12);

        //printf("BLT");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
        }
    
    //JAL
    else if (strcmp(opcode, "JAL") == 0) {
        sscanf(instruction, "%*s x%d, %d", &rd, &rs1);
        //rs1가 imm20 역할수행
        unsigned int out_code_num = 0b1101111 | (((rs1 << 1) & 0x8000) << 31 ) | (((rs1 & 0x7FE) >> 1) << 21) | (((rs1 & 0x00800) >> 11) << 20) | (((rs1 & 0xFF000) >> 12) << 12) | (rd << 7);

        //printf("JAL");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
        }
    
    //SW
    else if (strcmp(opcode, "SW") == 0) {
        sscanf(instruction, "%*s x%d, %d(x%d)", &rs2, &rd, &rs1);
        //rd  imm12 역할

        unsigned int out_code_num = 0b0100011 | (((rd & 0xFEF0) >> 5 ) << 25) | (rs2 << 20) | (rs1 << 15) | ((rd & 0x01F) << 7) | (0b010 << 12);

        //printf("SW");    //테스트용 문구
        print_binary(out_code_num, o_file);   //출력 
        }
    
    //라벨
    else if (strchr(opcode, ':')) {
        return;
    }
    
    //EXIT
    else if (strcmp(opcode, "EXIT") == 0) {
        print_binary(0xFFFFFFFF, o_file);
        for (int i = 0; i < label_count; i++) {
            free(label_arr[i].label_name);
        }
        exit(1);    //프로그램 강제종료.
    }
    
    else {
        printf("\ndddSyntax Error!!\n");
    }

    printf("instruction: %s\n", instruction);
    printf("rs1: %d, rs2: %d, rd: %d\n", rs1, rs2, rd);
}
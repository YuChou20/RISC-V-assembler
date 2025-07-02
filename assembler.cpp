#include<iostream>
#include<bitset>
#include<fstream>
#include<vector>
#include<string>
#include<utility>
#include<map>
#include<iomanip>
using namespace std;
#define cin infile
#define cout outfile
char ctemp[255];
string stemp;
vector <string> code;			//risc-V code
bitset<32> mach_code ,machtemp;
map<string, bitset<32> >registers;
map<string, unsigned long>label;
map<string, int> instruct_type;
map<string, bitset<32> >ins_template;

bitset<32> and_getValue(bitset<32> val1 , bitset<32> val2 , int shiftN){
	val2 &= val1;
	val2 >>= shiftN;
	return val2;
}

bitset<32> or_getValue(bitset<32> val1, bitset<32> val2, int shiftN) {
	val2 |= val1;
	if (shiftN >= 0) {			//positive num -> shift left
		val2 <<= shiftN;
	}
	else {						//negative num -> shift right
		val2 >>= shiftN;
	}
	return val2;
}

void build_instruction_data() {
	int type;
	string stemp2;
	ifstream ins_set("instruction_set.txt");
	while (ins_set >> stemp >> type >> stemp2)
	{
		instruct_type.insert(make_pair(stemp, type));
		bitset<32> bstemp(stemp2);
		ins_template.insert(make_pair(stemp,bstemp));
	}
	ins_set.close();
}

void build_register_data() {
	int regNo = 0;
	ifstream register_set("register.txt");
	while (register_set >> stemp >> regNo) {
		registers.insert(make_pair(stemp, regNo));
	}
	register_set.close();
}

void readcode() {
	ifstream infile("riscv_code.txt");
	while (infile.getline(ctemp, 255)) {
		stemp = ctemp;
		string::size_type pos = stemp.find(":");
		if (pos != stemp.npos) {
			label.insert(make_pair(stemp.substr(0, pos), code.size()));			//add label and save the NO of the instruction
			if (stemp.size() != pos + 1) {
				stemp = stemp.substr(pos + 1, stemp.size());
			}
			else {
				stemp = "";
			}
		}
		for (int i = 0; i < stemp.size(); ++i) {
			if (stemp[i] != ' ' && stemp[i] != '\t') {
				stemp = stemp.substr(i, stemp.size());
				break;
			}
		}
		if (stemp != "") {
			code.push_back(stemp);
		}
	}
	infile.close();
}

bitset<32> type_u(string ins,int index) {		//simm[31:12] + rd + opcode
	bitset<32> rd = 0;
	stemp = "";
	for (int i = 0; i < ins.size(); ++i) {		//lui rd,imm
		if (ins[i] == ',') {
			rd = registers[stemp];
			stemp = "";
		}
		else if (ins[i] != ' ') {
			stemp += ins[i];
		}
	}
	long immN = atoi(stemp.c_str());	//immN store imm
	return or_getValue(immN << 5, rd, 7);
}

bitset<32> type_uj(string ins,int index) {	//simm[20 | 10:1 | 11 | 19:12] + rd + opcode
	bitset<32>  rd = 0 , imm = 0, bittemp1 = 0 , bittemp2 = 0;
	stemp = "";
	for (int i = 0; i < ins.size(); ++i) {		//jal rd,rs1,offset
		if (ins[i] == ',') {
			rd = registers[stemp];
			stemp = "";
		}
		else if (ins[i] != ' ') {
			stemp += ins[i];
		}
	}
	long  immN = (label[stemp] - index) * 2;	//calculate offset
	bittemp1 = immN;
	bittemp1 <<= 12;									//avoid simm[63:21] != 0
	bittemp1 >>= 12;
	bittemp2 = and_getValue(bittemp1, 524288, 0);		//get the value of simm[20]
	imm = or_getValue(imm, bittemp2, -9);
	bittemp2 = and_getValue(bittemp1, 1023, 0);			//get the value of simm[10:1]
	imm = or_getValue(imm, bittemp2, 1);	
	bittemp2 = and_getValue(bittemp1, 1024, 10);		//get the value of simm[11]
	imm = or_getValue(imm, bittemp2, 8);
	bittemp2 = and_getValue(bittemp1, 522240, 11);		//get the value of simm[19:12]
	imm = or_getValue(imm, bittemp2, 5);
	return or_getValue(imm, rd, 7);
}

bitset<32> type_i(string ins,int index) {		//simm[11:0] + rs1 + funct3 + rd + opcode
	bitset<32> rs1 = 0, rd = 0, imm = 0;
	int mode = 0;
	string::size_type pos = ins.find("(");
	stemp = "";
	if (pos == ins.npos) {		//jalr rd,rs1,offset   addi rd,rs1,imm
		for (int i = 0; i < ins.size(); ++i) {
			if (ins[i] == ',') {
				if (mode == 0) {
					rd = registers[stemp];
				}
				else if (mode == 1) {
					rs1 = registers[stemp];
				}
				stemp = "";
				++mode;
			}
			else if (ins[i] != ' ') {
				stemp += ins[i];
			}
		}
		long immN = atoi(stemp.c_str());
		imm = immN;
	}
	else {		//lb rd,offset(rs1)
		for (int i = 0; i < ins.size(); ++i) {
			if (ins[i] == ',') {
				rd = registers[stemp];
				stemp = "";
			}
			else if (ins[i] == '(') {
				int immN = atoi(stemp.c_str());
				imm = immN;
				stemp = "";
			}
			else if (ins[i] == ')') {
				rs1 = registers[stemp];
			}
			else if (ins[i] != ' ') {
				stemp += ins[i];
			}
		}
	}
	imm = or_getValue(imm << 5, rs1, 8);
	return or_getValue(imm, rd, 7);
}

bitset<32> type_sb(string ins,int index) {	//simm[12 | 10:5] + rs2 + rs1 + funct3 + simm[4:1 | 11] + opcode
	bitset<32>  rs1 = 0, rs2 = 0, imm7 = 0,imm5 = 0,  bittemp1 = 0, bittemp2 = 2048;
	stemp = "";
	int mode = 0;
	for (int i = 0; i < ins.size(); ++i) {	//beq rs1,rs2,label
		if (ins[i] == ',') {
			if (mode == 0) {
				rs1 = registers[stemp];
			}
			else if (mode == 1) {
				rs2 = registers[stemp];
			}
			stemp = "";
			++mode;
		}
		else if (ins[i] != ' ') {
			stemp += ins[i];
		}
	}
	long immN= (label[stemp] - index) * 2;	//calculate the offset
	bittemp1 = immN;
	imm7 = and_getValue(bittemp1, 2048, 5) | and_getValue(bittemp1, 1008, 4);//get the value of simm[12 | 10:5]
	imm5 = (and_getValue(bittemp1, 15, 0) << 1) | and_getValue(bittemp1, 1024, 10);//get the value of simm[4:1 | 11]
	imm7 = or_getValue(imm7 << 5, rs2, 5);
	imm7 = or_getValue(imm7, rs1, 8);
	return or_getValue(imm7, imm5, 7);
}

bitset<32> type_s(string ins,int index) {// simm[11:5] + rs2 + rs1+ funct3 + simm[4:0] + opcode
	bitset<32> rs1 = 0, rs2 = 0,imm7,imm5;
	stemp = "";
	int mode = 0;
	for (int i = 0; i < ins.size(); ++i) {	//sb rs2,offset(rs1)
		if (ins[i] == ',') {
			rs2 = registers[stemp];
			stemp = "";
		}
		else if (ins[i] == '(') {
			int immN = atoi(stemp.c_str());
			imm7 = immN;	
			imm5 = imm7;
			imm7 >>= 5;		//get the value of simm[11:5]
			imm7 <<= 5;
			imm5 <<= 27;	//get the value of simm[4:0]
			imm5 >>= 27;
			stemp = "";
		}
		else if (ins[i] == ')') {
			rs1 = registers[stemp];
		}
		else if (ins[i] != ' ') {
			stemp += ins[i];
		}
	}
	imm7 = or_getValue(imm7, rs2, 5);
	imm7 = or_getValue(imm7, rs1, 8);
	return or_getValue(imm7, imm5, 7);
}

bitset<32> type_r(string ins, int index) {	//funct7 + rs2 + rs1 + funct3 + rd + opcode
	bitset<32> rs1 = 0, rs2 = 0, rd = 0;
	stemp = "";
	int mode = 0;
	for (int i = 0; i < ins.size(); ++i) {
		if (ins[i] == ',') {
			if (mode == 0) {
				rd = registers[stemp];
			}
			else if (mode == 1) {
				rs1 = registers[stemp];
			}
			stemp = "";
			++mode;
		}
		else if (ins[i] != ' ') {
			stemp += ins[i];
		}
	}
	rs2 = registers[stemp] << 5;
	rs2 = or_getValue(rs2, rs1, 8);
	return or_getValue(rs2, rd, 7);
}

bitset<32> type_select(string& ins_typ , string& ins , int index ) {
	switch (instruct_type[ins_typ]) {
		case 1:
			machtemp = type_u(ins, index);
			break;
		case 2:
			machtemp = type_uj(ins, index);
			break;
		case 3:
			machtemp = type_i(ins, index);
			break;
		case 4:
			machtemp = type_sb(ins, index);
			break;
		case 5:
			machtemp = type_s(ins, index);
			break;
		case 6:
			machtemp = type_r(ins, index);
			break;
	}
	return mach_code | machtemp;
}

void transfer_code() {
	ofstream outfile("machine_code.txt");
	for (int i = 0; i < code.size(); ++i) {
		stemp = code[i];
		for (int j = 0; j < stemp.size(); ++j) {
			if (stemp[j] != ' ' && stemp[j] != '\t') {		//remove space charactor
				stemp = stemp.substr(j, stemp.size());
				code[i] = stemp;
				int index = stemp.find(" ");
				string type = stemp.substr(0, index);
				stemp = stemp.substr(index + 1, stemp.size());
				mach_code = ins_template[type];
				mach_code = type_select(type,stemp,i);
				unsigned long ins = mach_code.to_ulong();
				cout << setw(12) << right << showbase << hex << ins << setw(40) << mach_code << "	" << left << code[i] << endl;
				break;
			}
		}
	}
	outfile.close();
}

int main() {
	build_instruction_data();
	build_register_data();
	readcode();
	transfer_code();
}
1071536 Project1 assembler readme

在執行程式前請確保資料夾底下包含以下txt檔:
	1.instruction_set.txt:
		內容為一個instruction type(add,addi...)、一個type的編號(1~6)以及一個對應32bits的數字(rd,rs1,rs2,imm預設0)
		type編號的不同會使不同的instruction丟入不同的function處理，下面會有更詳細的說明
		32bits的數字則是一個模板，執行時會陸續算出instruction的rd,rs1,rs2,imm....在通過or的方式得到machine code
		Ex:and: funct7:0000000 ,rs2 = 0 , rs1 = 0 ,  funct3:111 , rd = 0 , opcode = 0110011
		   and x3,x4,x5;  =>rs1 = 00100 , rs2 = 00101 , rd = 00011
		   and 6 00000000000000000111000000110011(模板)
		     or) 00000000010100100000000110000000
		---------------------------------------------
			 00000000010100100111000110110011
	2.register.txt:
		內容為一個欲使用的register名稱對應一個register的編號
		Ex:x0 0
		   x1 1
		目前附加檔案已包含rars中的所有可能(x0,x1,a0,t0,s0....)
	3.riscv_code.txt:
		此檔案的內容為使用者自行輸入，內容為欲轉換成machine code的risc V code
		格式為一個在rars上可執行的code，且符合以下條件:
			1.如欲在instruction前加label，則使用label後+冒號+" "(一個空格)+instruction
				Ex:loop: addi x2,x5,16
			2.如每個label單獨佔一行則為label+冒號(後面不可以有任何空格)
				Ex:loop:
				   addi x2,x5,16是
			3.每個instruction輸入格式為
				 i.運算元和第一個register中間有空格
				ii.之後都不能有空格(用逗號隔開)
				Ex:addi x2,x5,16 
				   sb x3,0(x5)
			4.不能使用tab
			5.所有字皆為小寫
			6.不支援任何註解
	4.machine_code.txt:
		此檔案可有可無，為此程式執行後的結果，在資料夾底下一開始若沒有此檔案，則執行過後會自動生成一個

cpp內的變數名稱:
	char ctemp[225]:char的temp，在讀檔的時候會使用
	vector<string>code:在riscv_code中的code每行以string的形式儲存至vector中
	bitset<32> mach_code:一個temp，在risc v轉換成machine code時會用到
	map<string, unsigned long>label:儲存每個label的名子和PC(這裡的PC是0,1,2,....)
	map<string, int> instruct_type:根據不同的instruction type分成
		1. u type:lui,auipc
		2.uj tupe:jal
		3. i type:jalr,lb,lh,lv,lbn,lhu,addi,slti,sltiu,xori,ori,andi,slli,srli,srai
		4.sb type:beq,bne,blt,bge,bltu,bgeu
		5. s type:sb,sh,sv
		6. r type:add,sub,sll,slt,sltu,xor,srl,sra,or,and
	map<string, bitset<32> >ins_template:將instrution_set.txt中的32bits數字以及對應的operator存入map中
						到時候在轉換的時候會以這個當做模板

程式執行階段:總共會做4件事情(function)分別為:
	1.呼叫bulid_instruction_data()，把instruction_set.txt的資料儲存到instruction_type和ins_template中
	2.呼叫build_register_data()，把register.txt中的資料儲存到registers中
	3.呼叫readcode()，把rsicv_code的資料去除label後儲存到code中(++PC)，有label時把label名稱和label的PC儲存到label(map)中，
	  單純只有label時PC不會增加
	4.呼叫transfer_code():
		在transfer_code這個function中，會先把每一行的code分成operator(type)和operand(stemp)並呼叫type_select()，
		並且透過type去ins_template(map)中找到operator對應的32bits machine code 模板並存到mach_code中
			在type_select中會根據剛剛切割的operator(type)去instruct_type(map)中找到對應的type類型並執行對應的function，
			這六個function都會分別地去找instruction的rs1,rs2,rd,imm...(funct7,funct3,opcode則已經在讀檔的時候就處理好了)，
			所有的function都是透過bitset來處理轉換machine code，function如下:
				1. u type:
					根據instruction找到simm[31:12]和rd並合併成一個數字，最後回傳這個數字
				2.uj tupe:
					根據instruction找到simm[20 | 10:1 | 11 | 19:12]和rd並合併成一個數字，最後回傳這個數字
					*uj type的imm是(label.PC - current_ins.PC)*4		//ex:jal
				3. i type:
					根據instruction找到simm[11:0](分成有括弧和沒括弧兩種case下去找offset)、rs1和rd並合併成一個數字，最後回傳這個數字
				4.sb type:
					根據instruction找到simm[12 | 10:5]、rs2、rs1和simm[4:1 | 11]並合併成一個數字，最後回傳這個數字
					*sb type的imm是(label.PC - current_ins.PC)*4		//ex:beq
				5. s type:
					根據instruction找到simm[11:5]、rs2、rs1和simm[4:0]並合併成一個數字，最後回傳這個數字
				6. r type:
					根據instruction找到rs2、rs1和rd1並合併成一個數字，最後回傳這個數字

			這六個function都會回傳一個32bits的數字(machtemp)，接著再回傳machtemp | mach_code的結果(轉換過後的machine code)
			*這邊的合併數字指的是把數字的每一bit放到對應的位置，最後會得到一個數字稱作合併(使用&,|,<<,>>來操作)

		得到回傳回來的值(mach_code)就將結果寫入mach_code.txt中
		重複上述動作來轉換每個code直到結束，結果都會一行一行儲存在mach_code中

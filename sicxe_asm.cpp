/*
    
    Kyle Kubis
    sicxe_asm.cpp
    
    CS530, Spring 2014
*/


#include "sicxe_asm.h"
using namespace std;

struct sicxe_asm::list_line {
    string line_number;
    string address;
    string label;
    string opcode;
    string operand;
    string machine_code;
    
    string format_line_number(){
        stringstream tmp;
        tmp << setw(15) << line_number;
        return tmp.str();
    }
    
    string format_address(){
        stringstream tmp;
        tmp << setfill('0') << setw(5) << address;
        stringstream tmmp;
        tmmp << setw(15) << tmp.str();
        return tmmp.str();
    }
    
    string format_label(){
        stringstream tmp;
        tmp << left << setw(15) << label;
        stringstream tmmp;
        tmmp << setw(20) << tmp.str();
        return tmmp.str();
    }
   
    string format_opcode(){
        stringstream tmp;
        tmp << left << setw(15) << opcode;
        return tmp.str();
    }
   
    string format_operand(){
        stringstream tmp;
        tmp << left << setw(longest_operand+5) << operand;
        return tmp.str();
    }   
   
    string format_machine_code(){
        stringstream tmp;
        tmp << left << setw(15) << str_to_upper(machine_code);
    return tmp.str();
    }
   
    list_line(){
    }
    
    list_line(string line, string a, string lbl, string opcd, string oper, string mac_code)
    {
        line_number = line;
        address = a;
        label = lbl;
        opcode = opcd;
        operand = oper;
        machine_code = mac_code;
    }
    
    list_line& operator=(list_line input){
        line_number = input.line_number;
        address = input.address;
        label = input.label;
        opcode = input.opcode;
        operand = input.operand;
        machine_code = input.machine_code;
        return *this;
    }
};

ostream& operator<<(ostream &out, sicxe_asm::list_line &line){
    out << line.format_line_number()
    << line.format_address()
    << line.format_label()
    << line.format_opcode()
    << line.format_operand()
    << line.format_machine_code();
    return out;
}


int main(int argc, char *argv[]) {
    if(argc != 2) {
        cout << "Error, you must supply the name of the file " <<
        "to process at the command line." << endl;
        exit(1);
    }
    sicxe_asm assembler(argv);
    return 0;
}

unsigned int sicxe_asm::longest_operand = 10;
sicxe_asm::sicxe_asm(char *argv[]){
    location_counter = 0;
    base = -1;
    file_name = argv[1];
    assemble_success = true;
    is_pass2 = false; 
    
    // intialize register map
    registers["A"] = 0;
    registers["X"] = 1;
    registers["L"] = 2;
    registers["B"] = 3;
    registers["S"] = 4;
    registers["T"] = 5;
    registers["F"] = 6;
    registers["PC"] = 8;
    registers["SW"] = 9;
    
    assemble();
}

void sicxe_asm::assemble(){
    file_parser *parser;
    parser = new file_parser(file_name);
    
    try{
        parser->read_file();
    }catch(file_parse_exception excpt){
        cout << "**Sorry, error " << excpt.getMessage() << endl;
        assemble_success = false;
    }
    cout << "Running pass 1" << endl;
    if(assemble_success)
        do_pass1(parser);
    is_pass2 = true;
    cout << "Resolving Forward References" << endl;
    if(assemble_success)
        resolve_forward_references();
    cout << "Running pass 2" << endl;
    if(assemble_success)
        do_pass2();
    if(assemble_success){
    	create_obj();
        print_list_file();
	print_obj_file();
        //sym_tab.print();
        cout << endl << "Complete" << endl;
    }else
        cout << endl << "Assemble Failed" << endl;
}

void sicxe_asm::do_pass1(file_parser *parser){
        for(cur_line_num = 1; cur_line_num <= parser->size(); cur_line_num++)
        try{
                pass1_process_line(parser->get_token(cur_line_num-1,0),
                parser->get_token(cur_line_num-1,1),
            parser->get_token(cur_line_num-1,2));
        }catch(opcode_error_exception excpt){
                print_error(excpt.getMessage());
            }catch(symtab_exception excpt){
                print_error(excpt.getMessage());
            }
}

void sicxe_asm::pass1_process_line(string label, string opcode, string operand){
    int current_size = 0;
    list_line cur_line;
    if(operand.size() > longest_operand)
        longest_operand = operand.size();
    cur_line = list_line(int_to_string(cur_line_num), int_to_hexstr(location_counter), label, opcode,operand, "");
    int i;
    i = check_asm_directive(str_to_upper(opcode));
    if(!process_program && i != 0){
        if(label != "" || opcode != "" || operand != "")//if we arnt processing it make sure we only have comment lines
            print_error("Action lines are not allowed before START or after END");
    }
    if(i >= 0 )
        process_directive(i, current_size, cur_line);
    else{
        if(process_program){
            if(opcode!= "")
                   current_size = opcode_tab.get_instruction_size(opcode);
            if(label != "")
                sym_tab.add_symbol(label, location_counter,true,true);
    }
    }
    list_file.push_back(cur_line);
    location_counter += current_size;
}

void sicxe_asm::do_pass2(){
        for(cur_line_num = 1; cur_line_num <= list_file.size(); cur_line_num++)
        try{
            pass2_process_line(list_file[cur_line_num-1]);
        }catch(opcode_error_exception excpt){
                print_error(excpt.getMessage());
            }catch(symtab_exception excpt){
                print_error(excpt.getMessage());
            }

}

void sicxe_asm::pass2_process_line(list_line& cur_line){
    int i;
    i = check_asm_directive(str_to_upper(cur_line.opcode));
    if(cur_line.opcode != "")
    	    if(i == 6)
            	process_directive_BASE(cur_line.label,cur_line.operand);
    	    else if(i == 7)
            	process_directive_NOBASE(cur_line.label,cur_line.operand);
            else if(i == 3)    
            	process_directive_WORD(cur_line, i);// i does nothing after this point so we use it as dummy
            else if(i == -1){    // we have something thats not a directive
            	switch(opcode_tab.get_instruction_size(cur_line.opcode)){
            	    case 1: 
                    	process_format1(cur_line);
            	    	break;
            	    case 2:
                    	process_format2(cur_line);
            	    	break;
            	    case 3:
                    	process_format3(cur_line);
            	    	break;
            	    case 4:
                    	process_format4(cur_line);
            	    	break;
                    default:
                    	// we might want to throw an error here
            	    	break;
            	}
            }
}

void sicxe_asm::print_list_file(){
    ofstream outfile;
    string tmp_filename;
    tmp_filename = grab_file_name(file_name) + ".lis";
    outfile.open(tmp_filename.c_str());
    outfile << "\t\t\t\t**" << file_name << "**" << endl;
    list_line line;
    line = list_line("Line#", "Address", "Label", "Opcode", "Operand", "Machine Code");
    outfile << line << endl;
    line = list_line("=====", "=======", "=====", "======", "=======", "============");
    outfile << line << endl << endl;
    for(vector<list_line>::iterator iter = list_file.begin(); iter != list_file.end(); iter++)
        outfile << *iter << endl;
    outfile.close();
}

void sicxe_asm::print_obj_file(){
    ofstream outfile;
    string tmp_filename;
    tmp_filename = grab_file_name(file_name) + ".obj";
    outfile.open(tmp_filename.c_str());
    for(vector<string>::iterator iter = obj_code.begin(); iter != obj_code.end(); iter++)
        outfile << *iter << endl;
    outfile.close();
}

int sicxe_asm::check_asm_directive(string opcode){
    opcode = str_to_upper(opcode);
    string s[] ={"START","END","BYTE","WORD","RESB","RESW","BASE","NOBASE","EQU"};
    
    for(unsigned int i=0; i<sizeof(s)/sizeof(string);i++)
        if(opcode == s[i])
            return i;
    return -1;
}

void sicxe_asm::process_directive(int directive_number, int &current_size,
    list_line &cur_line){
    if ((directive_number != 0) && !process_program)
        return;
    switch(directive_number){
        case 0:
            process_directive_START(cur_line.label, cur_line.operand, current_size);
            break;
    	case 1:
            process_directive_END(cur_line.label, cur_line.operand);
            break;
     	case 2:
            process_directive_BYTE(cur_line.label, cur_line.operand, cur_line.machine_code, current_size);
            break;
    	case 3:
            process_directive_WORD(cur_line, current_size);
            break;
    	case 4:
            process_directive_RESB(cur_line.label, cur_line.operand, current_size);
            break;
    	case 5:
            process_directive_RESW(cur_line.label, cur_line.operand, current_size);
            break;
    	case 6:
            process_directive_BASE(cur_line.label, cur_line.operand);
            break;
    	case 7:
            process_directive_NOBASE(cur_line.label, cur_line.operand);
            break;
    	case 8:
            process_directive_EQU(cur_line);
            break;
    	default:
            break;
    }
}

void sicxe_asm::process_directive_START(string label, string operand, int &current_size){
    program_name = str_to_upper(label);
    operand = str_to_upper(operand);
    if(operand == "")
        print_error("No operand associated with start");
    
    if (parse_hexstr(operand)){
        if(is_hexstr(operand))
            current_size = hexstr_to_int(operand);
    else
        print_error("Invalid Hex number");
    }else if(is_number(operand))
        current_size = string_to_int(operand);
    else
        print_error("Directive START requires a numeric operand");
    process_program = true;
    start_address = current_size;
}

void sicxe_asm::process_directive_END(string label, string operand){
    if(label != "")
        sym_tab.add_symbol(label, location_counter,true,true);
    process_program = false;
    end_address=location_counter;
}

void sicxe_asm::process_directive_BYTE(string label, string operand, string &mac_code, int &current_size){
    if(label != "")
        sym_tab.add_symbol(label,location_counter,true,true);
    current_size = parse_BYTE_operand(operand, mac_code);
}

void sicxe_asm::process_directive_WORD(list_line &line, int &current_size){
    if(line.label != "" && !is_pass2)
        sym_tab.add_symbol(line.label,location_counter,true,true);
    int total;
    bool isRelative;
    if(parse_operand(line,total,isRelative)){
        line.machine_code = int_to_mac_code(total,6);
        current_size = 3;
    	if(isRelative)
	    add_modification_record(line);
    }else if(is_pass2)
    	print_error("unable to resolve operand");
}

void sicxe_asm::process_directive_RESB(string label, string operand, int &current_size){
    if(label != "")
        sym_tab.add_symbol(label,location_counter,true,true);
    if (parse_hexstr(operand)){
        if(is_hexstr(operand))
            current_size = hexstr_to_int(operand);
    else
        print_error("Invalid Hex number");
    }else if(is_number(operand))
        current_size = string_to_int(operand);
    else
    print_error("Directive RESB requires a numeric operand");
}

void sicxe_asm::process_directive_RESW(string label, string operand, int &current_size){
    if(label != "")
        sym_tab.add_symbol(label,location_counter,true,true); 
    if (parse_hexstr(operand)){
        if(is_hexstr(operand))
        current_size = hexstr_to_int(operand) * 3;
    else
        print_error("Invalid Hex number");
    }else if(is_number(operand))
        current_size = string_to_int(operand) * 3;
    else
    print_error("Directive RESW requires a numeric operand");
}

void sicxe_asm::process_directive_NOBASE(string label, string operand){
    if(label != "")
        sym_tab.add_symbol(label,location_counter,true,true);
    if (!(parse_hexstr(operand)  && is_hexstr(operand) || is_number(operand)))
        if (is_pass2 || sym_tab.isDefined(operand)){
	    bool tmp;
            sym_tab.get_value(operand,tmp);
	}
    base = -1;
}

void sicxe_asm::process_directive_BASE(string label, string operand){
    if(label != "")
        sym_tab.add_symbol(label,location_counter,true,true);
    if (!(parse_hexstr(operand)  && is_hexstr(operand) || is_number(operand)))
        if (is_pass2 || sym_tab.isDefined(operand)){
	    bool tmp;
            base=sym_tab.get_value(operand,tmp);
            return;
    	}
    base = -1;
}

int sicxe_asm::parse_BYTE_operand(string operand, string &mac_code){
    string characters;
    int char_size=0;
    
    if (toupper(operand[0]) == 'C'){ 
       for(int i = 2; operand[i] != '\''; i++)
           characters += operand[i];
    mac_code = str_to_mac_code(characters);
       char_size = characters.size();
       return char_size;
    }else if (toupper(operand[0]) == 'X' ){ //Checking for hex
        for(int i = 2; operand[i] != '\''; i++)
           characters += operand[i];
    //Insure that there is an even number of characters
         if ( characters.size() % 2 == 0 && is_hexstr(characters)){
            char_size = characters.size();
            char_size = char_size/2;
        mac_code = characters;
            return char_size; 
    }
    }
    print_error("BYTE directive not valid."); //show invalid
    return char_size;
}

void sicxe_asm::process_directive_EQU(list_line line){
    if(line.label == "")
        print_error("Label required for EQU directive");
    int total;
    bool isRelative;
    if(parse_operand(line,total,isRelative)){
    	sym_tab.add_symbol(line.label,total,true,isRelative);
    }else{
    	add_undefined_symbol(line.label,line);
    }
}

bool sicxe_asm::parse_operand(list_line line,int &total,bool &isRelative){
    string tmp_operand = line.operand;
    cur_line_num=string_to_int(line.line_number);
    char *split_operand_plus, *split_operand_minus;
    const char *delim_plus = "+", *delim_minus = "-";
    int tmpValue=0,relativeCounter=0;
    bool tmpIsRelative=false;
    vector<string> tmp;
    vector<pair<string,bool> > operand;    // true = '+', false = '-'
    vector<pair<int,bool> > values;

    total=0;
    
    if(tmp_operand[0] == '+' || tmp_operand[0] == '-')
        print_error("Invalid operand");
    split_operand_plus=strtok(const_cast<char*>(tmp_operand.c_str()),delim_plus);
    while(split_operand_plus!=NULL){ // split everything by + and add it to vector
        if(split_operand_plus[0]=='-')
            print_error("invalid operand: "+line.operand);
        tmp.push_back(split_operand_plus);
        split_operand_plus=strtok(NULL,delim_plus);//get next part
    }
    for(unsigned int i=0;i<tmp.size();i++){ // split remainig by minus
        split_operand_minus=strtok(const_cast<char*>(tmp[i].c_str()),delim_minus);
        operand.push_back(pair<string,bool>(split_operand_minus,true));//foo-tmp (foo is pos)
        split_operand_minus=strtok(NULL,delim_minus);// get the next part
        while(split_operand_minus!=NULL){
            operand.push_back(pair<string,bool>(split_operand_minus,false));
            split_operand_minus=strtok(NULL,delim_minus);// get the next part
        }
    }
    for(unsigned int i=0;i<operand.size();i++){
    	//cout << "Part" << i << ":" << operand[i].first << endl;
        if(!get_operand_value(operand[i].first,tmpValue,tmpIsRelative))
            return false;
        if(operand[i].second){
            total+=tmpValue;
            relativeCounter+=tmpIsRelative;
        }else{
            total-=tmpValue;
            relativeCounter-=tmpIsRelative;
        }
    }
    if(relativeCounter > 1 || relativeCounter < 0)
        print_error("invalid operand: "+line.operand);
    isRelative=relativeCounter;
    return true;
}

bool sicxe_asm::get_operand_value(string operand, int &value,bool &isRelative){
    if (parse_hexstr(operand)){
        if(is_hexstr(operand)){
            isRelative=false;
            value=hexstr_to_int(operand);
            return true;
        }else
            print_error("Invalid Hex number");
    }else if(is_number(operand)){
        isRelative=false;
        value=string_to_int(operand);
        return true;
    }else{
        if(is_pass2){
            value=resolve_reference(operand, isRelative);
            return true;
        }else{
            if(sym_tab.isDefined(operand)){
                value=sym_tab.get_value(operand,isRelative);
                return true;
            }
        }
    }
    //cout << "HERE" << endl;
    return false;
}

void sicxe_asm::resolve_forward_references(){
    int value;
    bool isRelative;
    string firstKey;
    while(undefined_symbols.size()>0){
        firstKey = undefined_symbols.begin()->first;
        firstKey = str_to_upper(firstKey);
    	//cout << firstKey << "," << undefined_symbols.size() << endl;
	value=resolve_reference(firstKey,isRelative);
        set_symbol_value(firstKey, value, isRelative);
    }
}

int sicxe_asm::resolve_reference(string key, bool &isRelative){
    key = str_to_upper(key);
    //cout << "Resolve: " << key << endl;
    if(!sym_tab.exists(key)){
        print_error("Symbol: " + key + " is not defined");
    }else if(sym_tab.isDefined(key)){ // check if defined
        return sym_tab.get_value(key,isRelative);
    }else{
        //if not defined see if we can define it
        int value=0;
	if(undefined_symbols.find(key) == undefined_symbols.end())
	    print_error("unable to define " + key);
	list_line tmp = (*undefined_symbols.find(key)).second;
    	//cout << "Resolve line: " << tmp << endl;
        if (!parse_operand(tmp,value,isRelative)){
            value = resolve_reference(key,isRelative);
        }
	set_symbol_value(key,value,isRelative);
        return value;
    }
    return 0;
}

void sicxe_asm::set_symbol_value(string key,int value,bool isRelative){
    sym_tab.set_value(key,value,isRelative);
    undefined_symbols.erase(key);
}

void sicxe_asm::add_undefined_symbol(string key, list_line line){
    //cout << "added Symbol: " << key << endl;
    //cout << line << endl;
    key = str_to_upper(key);
    sym_tab.add_symbol(key,0,false,false);
    if(undefined_symbols.find(key) != undefined_symbols.end())
        throw symtab_exception("Cannot use duplicate keys: " + key); 
    undefined_symbols[key]=line; 
}

void sicxe_asm::process_format1(list_line &cur_line){
    if(cur_line.operand != "")
        print_error("format one opcode cannot have an operand");
    cur_line.machine_code=opcode_tab.get_machine_code(cur_line.opcode);
}

void sicxe_asm::process_format2(list_line &cur_line){
    string opcode_upper = str_to_upper(cur_line.opcode);
    string operand_upper = str_to_upper(cur_line.operand);
    int mac_code = hexstr_to_int(opcode_tab.get_machine_code(cur_line.opcode));
    int tmp = 0;
    mac_code <<= 8;
    if(opcode_upper == "CLEAR" ||
        opcode_upper == "TIXR"){// check for only one operand case
    if (!process_register(operand_upper, tmp))
         return;
    tmp <<= 4;
    mac_code += tmp;
    cur_line.machine_code = int_to_mac_code(mac_code,4);    
        
    }else if(opcode_upper == "SVC"){ // must be an integer
        if(!process_format2_integer(operand_upper, tmp))
        return;
    tmp <<= 4;
    mac_code += tmp;
    cur_line.machine_code = int_to_mac_code(mac_code,4);
    
    }else if(opcode_upper == "SHIFTL" || opcode_upper == "SHIFTR"){
        string r1,n;
        parse_format2_operand(operand_upper, r1,n);
    if (!process_register(r1, tmp))
         return;
        tmp <<= 4;
    mac_code += tmp;
    if(!process_format2_integer(n, tmp))
        return;
        tmp --;
    mac_code += tmp;
    cur_line.machine_code = int_to_mac_code(mac_code,4);
    }else{
        //all of these should have two operands seperated by a ','
        string r1,r2;
        parse_format2_operand(operand_upper, r1,r2);
    if (!process_register(r1, tmp))
         return;
        tmp <<= 4;
    mac_code += tmp;
    if (!process_register(r2, tmp))
         return;
    mac_code += tmp;
    cur_line.machine_code = int_to_mac_code(mac_code,4);
    }
}

void sicxe_asm::process_format3(list_line &cur_line){
    int temp=0;
    int opcode_value=hexstr_to_int(opcode_tab.get_machine_code(cur_line.opcode));
    temp+=opcode_value << 16;
    if(opcode_value==76){
        temp|=nbit;
    	temp|=ibit;
    	cur_line.machine_code=int_to_mac_code(temp,6);
    }else{
    	string tmp = cur_line.operand;
    	get_nix_bitflags(cur_line.operand,temp);
    	parse_format3_operand(cur_line, temp);
	cur_line.operand=tmp;
    	cur_line.machine_code=int_to_mac_code(temp,6);
    }    
}

void sicxe_asm::process_format4(list_line &cur_line){
    int shifted_opcode =0;
    int opcode_value=hexstr_to_int(opcode_tab.get_machine_code(cur_line.opcode));
    shifted_opcode+=opcode_value << 16;
    if(opcode_value == 76){ //rsub
        shifted_opcode |= nbit;
    	shifted_opcode |= ibit;
    	shifted_opcode |= ebit;
    	cur_line.machine_code = int_to_mac_code(shifted_opcode,8);
    }else{
    	string tmp = cur_line.operand;
    	set_bit_flags_format4(cur_line.operand,shifted_opcode);
    	shifted_opcode <<= 8;
    	parse_format4_operand(cur_line,shifted_opcode);
	cur_line.operand=tmp;
    	cur_line.machine_code=int_to_mac_code(shifted_opcode,8);
    }
    
}

void sicxe_asm::parse_format2_operand(string operand, string &r1, string &r2){
    for(int unsigned i = 0; i < operand.size(); i++){
        if(operand[i] == ','){
        r1 = operand.substr(0, i);
        r2 = operand.substr(i+1);
        return;
    }
    }
    print_error("format 2 operand was not seperated by comma");
}

bool sicxe_asm::process_register(string r1, int &tmp){
    if(registers.find(r1) == registers.end()){
        print_error(r1 + " is not a valid register");
        return false;
    }
    tmp = registers[r1];
    return true;
}

bool sicxe_asm::process_format2_integer(string n, int &tmp){
    if (parse_hexstr(n)){
        if(is_hexstr(n))
            tmp = hexstr_to_int(n);
    }else if(is_number(n)){
        tmp = string_to_int(n);
    }else{
        print_error("SVC must have a numeric operand");
        return false;
    }
    if(tmp < 1 || tmp > 16){
        print_error("SVC requires a number between 1 and 16");
    return false;
    }
    return true;
}

void sicxe_asm::parse_format3_operand(list_line &current_line,int &bit_flags){
    string check=current_line.operand;
    int total=0;
    bool isRelative;
    if(!parse_operand(current_line,total,isRelative))
        print_error("Unable to resolve operand");
    if((bit_flags & 0x30000)==ibit || (bit_flags & 0x30000)==nbit){
        /*if (parse_hexstr(current_line.operand)){
            if(is_hexstr(current_line.operand)){
                bit_flags+=hexstr_to_int(current_line.operand);
            }else
                print_error("Invalid Hex number");
        }else if(is_number(current_line.operand)){
            bit_flags+=string_to_int(current_line.operand);
        }else
            bit_flags+=(0x00000FFF & (get_offset(current_line,bit_flags)));*/
        if (isRelative){
            bit_flags+=(0x00000FFF & (get_offset(current_line,bit_flags)));
        }else{
            bit_flags+=total;
	}    
    }else{
        bit_flags+=(0x00000FFF & (get_offset(current_line,bit_flags)));
    }
}

void sicxe_asm::parse_format4_operand(list_line &current_line, int &bit_flags){
    string check=current_line.operand;
    int total;
    bool isRelative;
    parse_operand(current_line,total,isRelative);
    if(isRelative)
    	add_modification_record(current_line);
    bit_flags += total;
}

void sicxe_asm::set_bit_flags_format4(string &operand, int &bit_flags){
    get_nix_bitflags(operand, bit_flags);
    bit_flags |= ebit;
}

void sicxe_asm::get_nix_bitflags(string &operand, int &bit_flags){
    if (check_for_nbit(operand))
        bit_flags |= nbit;
    else if (check_for_ibit(operand))
        bit_flags |= ibit;
    else{
        bit_flags |= nbit;
        bit_flags |= ibit;
        if(check_for_xbit(operand))
            bit_flags |= xbit;
    }
}

int sicxe_asm::get_offset(list_line &current_line,int &bit_flags){
    int total;
    bool isRelative;
    parse_operand(current_line,total,isRelative);
    if((check_relative_range(current_line))==1){
        bit_flags|=pbit;
        return(total)-((hexstr_to_int(current_line.address)+3));
    }
    else if((check_relative_range(current_line))==2){
        if(base>0){
            bit_flags|=bbit;
            return (total)-base;
        }else
            print_error("Base is not set");
    }
    else
        print_error("The displacement is too big for format 3");
    return -1;
}

int sicxe_asm::check_relative_range(list_line line){
    int total;
    bool isRelative;
    parse_operand(line,total,isRelative);
    int diff=(total)-(hexstr_to_int(line.address));
    if(diff>=(-2048) && diff<=4095){
         if(diff>=(-2048) && diff<=2047){
        return 1;
        }
    else if(diff>=0 && diff<=4095){ //is it greater, or greater/or equal to 0?
    	//cout << total << "," << base << endl;
        return 2;
        }
    }
    return -1;
}


bool sicxe_asm::check_for_nbit(string &operand){
    if (operand == "")
        print_error("Null Operand");
    
    if(operand[0] == '@'){
        if(operand.size() > 1){
            operand = operand.substr(1);
            return true;
        }else
            print_error("No Operand after @");
    }
    return false; 
}

bool sicxe_asm::check_for_ibit(string &operand){
    if (operand == "")
        print_error("Null Operand");
    
    if(operand[0] == '#'){
        if(operand.size() > 1){
            operand = operand.substr(1);
            return true;
        }else
            print_error("No Operand after #");
    }
    return false;
}

bool sicxe_asm::check_for_xbit(string &operand){
        if (operand == "")
        print_error("Null Operand");
    if(operand.size() > 2){
        if((operand[operand.size()-2] == ',') && ((operand[operand.size()-1] == 'X') || (operand[operand.size()-1] == 'x'))){
        operand = operand.substr(0,operand.size()-2);
        return true;
    }
    }
    return false;
}

void sicxe_asm::add_modification_record(list_line line){
    string tmp = "";
    tmp += "M";
    if(str_to_upper(line.opcode) == "WORD"){
    	tmp+=line.address;
	tmp+="06";
    }else if (line.opcode[0] == '+'){
    	int tmpAddress;
	tmpAddress=hexstr_to_int(line.address);
	tmpAddress++;
	tmp+=str_to_upper(int_to_mac_code(tmpAddress,6));
	tmp+="05";
    }else{
    	cur_line_num=string_to_int(line.line_number);
    	print_error("invalid type for modification record");
	return;
    }
    //label and sign are only needed if we use different control sections
    /*//we have the adress and size set now just add sign and label
    if(sign)
    	tmp+="+";
    else
    	tmp+="-";
    tmp+=substr(0,8);//make sure the label is the correct size*/
    mod_records.push_back(tmp);
}

void sicxe_asm::create_obj(){
   
    vector<list_line>::iterator iter;
    string cur_text_record="";
    string tmp_obj="";
   
    for(iter = list_file.begin(); iter != list_file.end() && str_to_upper(iter->opcode) != "START"; iter++);
    obj_code.push_back(set_header());
    int cur_address = start_address;
    
    for(;iter != list_file.end() &&  str_to_upper(iter->opcode) != "END"; iter++){
    	if(!set_test_record(cur_text_record,*iter)){
    	    //end of current text record
	    if(cur_text_record.size()==0)
	    	continue;
    	    tmp_obj += "T";
	    tmp_obj+=int_to_mac_code(cur_address,6);
	    tmp_obj+=int_to_mac_code(cur_text_record.size()/2,2);
	    tmp_obj+=cur_text_record;
    	    obj_code.push_back(str_to_upper(tmp_obj));
    	    cur_text_record ="";
    	    tmp_obj="";
    	    cur_address = hexstr_to_int(iter->address);
    	    set_test_record(cur_text_record,*iter);
    	}
    }
    for(vector<string>::iterator iter2 = mod_records.begin(); iter2 != mod_records.end(); iter2++)
    	obj_code.push_back(*iter2);
    obj_code.push_back(str_to_upper(set_end_record()));
}

string sicxe_asm::set_header(){
    string tmp;
    stringstream ss;
    ss  << left << setw(6) << program_name.substr(0,6);
    tmp+= "H";
    tmp+=ss.str();
    tmp+=int_to_mac_code(start_address,6);
    tmp+=int_to_mac_code((end_address-start_address),6);
    return str_to_upper(tmp);
}

bool sicxe_asm::set_test_record(string &cur_text_record,list_line cur_line){
    if(check_asm_directive(str_to_upper(cur_line.opcode)) != -1)
    	return false;
    if(cur_line.opcode == "")
    	return false;
    if (cur_text_record.size()+cur_line.machine_code.size() >60)
    	return false;
    else
    	cur_text_record+= cur_line.machine_code;
    return true;
}

string sicxe_asm::set_end_record(){
    return "E"+int_to_mac_code(start_address,6);
}
void sicxe_asm::print_error(string error_msg){
    if(cur_line_num!= line || error_msg!=msg){ 
        cout << "**Sorry Error, Line: " << int_to_string(cur_line_num) << " " << error_msg << endl;
    line=cur_line_num;
    msg=error_msg;
    }
    assemble_success = false;
}

bool sicxe_asm::parse_hexstr(string &input){
    if(input.size() > 0 && input[0] == '$'){
        input = input.substr(1);
    return true;
    }
    return false;
}

string sicxe_asm::int_to_mac_code(int x, int size){
    stringstream tmp;
    tmp << setfill('0') << setw(size) << hex << x;
    return tmp.str();
}

string sicxe_asm::str_to_mac_code(string s){
    stringstream tmp;
    for(string::iterator str_iter = s.begin(); str_iter != s.end(); str_iter++){
        tmp << hex << (int)(*str_iter);
    }
    return tmp.str();
}

string sicxe_asm::grab_file_name(string input){
    return input.substr(0,input.size()-4);
}

bool is_hexstr(string input){
    if (input == "")
        return false;
    for(string::iterator str_iter = input.begin(); str_iter != input.end();str_iter++)
         if(!isxdigit(*str_iter))
            return false;
    return true;
}

int string_to_int(string input){
    int result;
    stringstream(input) >> result;
    return result;
}

bool is_number(string input){
    for(string::iterator str_iter = input.begin(); str_iter != input.end(); str_iter++)
        if(!isdigit(*str_iter))
        return false;
    return true;

}

string str_to_upper(string input){
    string::iterator str_iter;
    for(str_iter = input.begin(); str_iter != input.end(); str_iter++)
        *str_iter = toupper(*str_iter);
    return input;
}

string int_to_string(int x){
    stringstream ss;
    ss << x;
    return ss.str();
}

string int_to_hexstr(int input){  
    stringstream ss;
    ss << hex << input;
    return str_to_upper(ss.str());
}

int hexstr_to_int(string input){ 
    int x; 
    stringstream ss;
    ss << hex << input;
    ss >> x;
    return x;
}





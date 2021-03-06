﻿#if _MSC_VER >= 1800
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <regex>
#include <time.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux
#include <sys/stat.h>
#include <dirent.h>
#endif
#ifdef WIN32
#include <io.h>
#include <tchar.h>
#include <windows.h>
#endif
#include <iostream>
#include <vector>
#ifdef __linux
#define MAXBYTE 0xff
#define MAX_PATH 360
#define DEFAULT_PATH "/usr/include/"
#endif
#define FUNCTION_OPTION "-f"
#define MACRO_OPTION "-m"
#define STRUCTURE_OPTION  "-s"
#define TYPEDEF_OPTION "-t"
#define UNION_OPTION "-u"
#define ENUM_OPTION "-e"
#define CLASS_OPTION "-c"
#define NAMESPACE_OPTION "--ns"
#define NO_SEARCH_FILE_OPTION "-nsf"
#define NO_SEARCH_PATH_OPTION "-nd"
#define PATH_OPTION "-d"
#define FILE_OPTION "-sf"
enum Search_Type{
	INVALID_VAL = -1,
	searchFun = 0,
	searchMacro,
	searchStruct,
	searchTypeDefine,
	searchUnion,
	searchEnum,
	searchClass,
	searchNameSpace
};
struct search_infor{
	std::string spath;//search path
	std::vector<std::string> sfname;//search file name
};
//bool g_bFun;
int getfilelen(FILE *fp);
void getdir(const char *root, std::vector<search_infor>&path);
bool get_val_in_line(int argc, char *argv[], const std::string&lpsstr, std::string&lpstr, int32_t start = 1);
int get_index_in_line(int argc, char *argv[], const std::string&str, int32_t start = 1);
void get_option_val(int argc, char *argv[], const std::string&opt, std::vector<std::string>&out);
int linage(const std::string&content);
void help();
bool isCommit(const char *pStr, int len);
bool isOption(int argc, char *argv[], int index);
// bool isFun(const char *lpstr, int str_size, const char *fun_name);
bool isInvalid(int argc, char *argv[], const std::vector<std::string>&option);
const char *movepointer(const char *p, char ch, bool bfront);
void remove_comment(char *content);
void removeSame(const std::vector<std::string>&in, std::vector<std::string>&out);
// void removeSame(const std::vector<std::string>&in, std::vector<search_infor>&out);
void search(const std::string&cPath, const std::string&filename, const std::string&lpstr, void(*fun)(const std::string&content, const std::string&lpstr, std::vector<std::string>&str));
void search_fun(const std::string&content, const std::string&lpstr, std::vector<std::string>&str);
void search_macro(const std::string&content, const std::string&lpstr, std::vector<std::string>&str);
void search_struct(const std::string&content, const std::string&lpstr, std::vector<std::string>&str);
void search_type_define(const std::string&content, const std::string&lpstr, std::vector<std::string>&str);
#ifdef WIN32
void SetTextColor(WORD color);
#endif
std::string searchStructString = "struct";
//char *strrpc(char *str,char *oldstr,char *newstr);
void removeSameFile(const std::vector<std::string>&in, std::vector<search_infor>&out){
	for (size_t i = 0; i < out.size(); ++i){
		removeSame(in, out[i].sfname);
	}
}
void removeSamePath(const std::vector<std::string>&in, std::vector<search_infor>&out){
	for (size_t i = 0; i < in.size(); ++i){
		for (size_t j = 0; j < out.size(); ++j){
			if(in[i] == out[j].spath){
				out.erase(out.begin() + j);
			}	
		}
	}
}
int main(int argc, char *argv[], char *envp[]){//envp环境变量表
	double getdir_totaltime = 0.0f, search_totaltime = 0.0f;
	clock_t getdir_start, getdir_finish, search_file_start, search_file_finish;//time count
	
	const std::vector<std::string> option = { FUNCTION_OPTION, MACRO_OPTION,  STRUCTURE_OPTION, TYPEDEF_OPTION, UNION_OPTION, ENUM_OPTION, CLASS_OPTION, NAMESPACE_OPTION };//, "-d", "-n"
	if(isInvalid(argc, argv, option)){
		printf("parameter insufficient:\n");
		help();
		return -1;
	}
	
	std::vector<std::string> rootPath;
	std::vector<std::string> searchFile;
	std::vector<std::string> noSearchPath;
	std::vector<std::string> noSearchFile;// = { "vulkan_core.h" };
	/*
		目前的程序，不希望用户通过-n选项指定其他路径的文件。需要指定其他路径，必须通过-d+-sf选项
		*.h

		如果用户指定了路径和文件名。但希望搜索该路径下所有文件以及搜索指定的文件名？
		目前的程序都是直接将路径和指定的文件名绑定
	*/

	get_option_val(argc, argv, FILE_OPTION, searchFile);
	get_option_val(argc, argv, NO_SEARCH_PATH_OPTION, noSearchPath);
	get_option_val(argc, argv, NO_SEARCH_FILE_OPTION, noSearchFile);
	//上面获取的有可能是正则表达式,
	get_option_val(argc, argv, PATH_OPTION, rootPath);

	if(rootPath.empty()){//用户未指定目录就从默认的目录查找
#ifdef __linux
		rootPath.push_back(DEFAULT_PATH);
#endif
#ifdef WIN32
		rootPath.push_back(getenv("include"));
		if(rootPath.empty()){
			printf("not found environment variable:include, please specify include path or other path\n");
			exit(-1);
		}
#endif
	}
	removeSame(noSearchFile, searchFile);
	void (*fun[])(const std::string&content, const std::string&lpstr, std::vector<std::string>&str) = {
		search_fun,
		search_macro,
		search_struct,//class、enum、namespace
		search_type_define,
	};
	int total_file = 0;
	//支持查询多个相同选项：-f strcpy printf -m MAXBYTE -m A
	for (size_t i = 0; i < option.size(); ++i){
		//判断需要查询的类型
		std::vector<std::string> findStr;
		get_option_val(argc, argv, option[i], findStr);
		//收集相关信息
		Search_Type funIndex = (Search_Type)i;
		if(funIndex > searchTypeDefine){
			const char *s[] = { "union", "enum", "class", "namespace" };
			searchStructString = s[funIndex - 4];
			funIndex = searchStruct;//------note:
		}
		if(searchFile.empty()){
			//没有指定文件名称，需要查找目录下的所有文件名
			std::vector<std::vector<search_infor>>path(2);
			for(int strIndex = 0; strIndex < findStr.size(); ++strIndex){
				for (size_t j = 0; j < rootPath.size(); ++j){
					getdir_start = clock();		
					getdir(rootPath[j].c_str(), path[j]);
					getdir_finish = clock();
					getdir_totaltime = (double)(getdir_finish - getdir_start) / CLOCKS_PER_SEC;
					search_file_start = clock();
					removeSamePath(noSearchPath, path[j]);
					removeSameFile(noSearchFile, path[j]);
					for(int k = 0; k < path[j].size(); ++k){
						for(int l = 0; l < path[j][k].sfname.size(); ++l)
							search(path[j][k].spath, path[j][k].sfname[l], findStr[strIndex], fun[funIndex]);
					}
					search_file_finish = clock();
					search_totaltime = (double)(search_file_finish - search_file_start) / CLOCKS_PER_SEC;
				}
			}
			//get file number			
			for(int i = 0; i < path.size(); ++i){
				for (size_t j = 0; j < path[i].size(); ++j){
					total_file += path[i][j].sfname.size();
				}			
			}
		}
		else{
			search_file_start = clock();
			total_file = searchFile.size() * rootPath.size();
			for(int strIndex = 0; strIndex < findStr.size(); ++strIndex){
				for (size_t j = 0; j < rootPath.size(); ++j){
					for (size_t k = 0; k < searchFile.size(); ++k){
						search(rootPath[j], searchFile[k], findStr[strIndex], fun[funIndex]);
					}
				}
			}
			search_file_finish = clock();
			search_totaltime = (double)(search_file_finish - search_file_start) / CLOCKS_PER_SEC;
		}
	}
	std::cout << "-----------------------------" << std::endl;
	std::cout << "search path:";
	for (size_t i = 0; i < rootPath.size(); ++i){
		std::cout << rootPath[i] << "\t";
	}
	std::cout << "\n";
	std::cout << "total file:" << total_file << ";get directory time:" << getdir_totaltime << ";search file time:" << search_totaltime << std::endl;
	return 0;
}
void removeSame(const std::vector<std::string>&in, std::vector<std::string>&out){
	for (size_t j = 0; j < in.size(); ++j){
		for (size_t i = 0; i < out.size(); ++i){
			if(out[i] == in[j]){
				out.erase(out.begin() + i);
				break;
			}
		}		
	}
}
/*{{{*/
void getdir(const char *root, std::vector<search_infor>&path){
	search_infor infor;
	infor.spath = root;
#ifdef __linux
	DIR *d;
	dirent *file = NULL;
	if(!(d = opendir(root))){
		perror("opendir error");
		printf("path is %s\n", root);
		return;
	}
	while((file = readdir(d))){
		if(strcmp(file->d_name, ".") && strcmp(file->d_name, "..")){
			if(file->d_type == DT_DIR){
				char szPath[MAX_PATH] = {0};
				if('/' != infor.spath[infor.spath.length() - 1])
					sprintf(szPath, "%s/%s", infor.spath.c_str(), file->d_name);
				else
					sprintf(szPath, "%s%s", infor.spath.c_str(), file->d_name);
				getdir(szPath, path);
			}
			else{
				infor.sfname.push_back(file->d_name);
			}
		}
	}
	closedir(d);
#endif
#ifdef WIN32
	long fHandle = 0;
	struct _tfinddata_t fa = { 0 };
	TCHAR cPath[MAX_PATH] = { 0 };
	if('\\' == root[strlen(root) - 1])
		_stprintf(cPath, _T("%s*.*"), root);
	else
		_stprintf(cPath, _T("%s\\*.*"), root);
	fHandle = _tfindfirst(cPath, &fa);
	if (-1 == fHandle) {
		perror(_T("findfirst error"));
		printf(_T("path is %s\n"), cPath);
		return;
	}
	else {
		do {
			if (_A_SUBDIR == fa.attrib) {
				//目录
				if (_tcscmp(fa.name, _T(".")) && _tcscmp(fa.name, _T(".."))) {
					if('*' != cPath[strlen(cPath) - 1])
						_stprintf(cPath, _T("%s\\%s"),cPath, fa.name);
					else
						_stprintf(cPath, _T("%.*s\\%s"),strlen(cPath) - strlen("\\*.*"), cPath, fa.name);
					getdir(cPath, path);
					memset(&cPath[strlen(cPath) - strlen(fa.name) - 1], 0, strlen(cPath) - strlen(fa.name));
				}
			}
			else{
				infor.sfname.push_back(fa.name);
			}
		} while (!_tfindnext(fHandle, &fa));
	}
#endif
	path.push_back(infor);
}
/*}}}*/
/*{{{*/
bool isCommit(const char *pStr, int len){
	if(!memchr(pStr, '/', len) && !memchr(pStr, '*', len))return false;
	for(int i = 0; i < len - 1; ++i){
		if(isalpha(pStr[i]))break;
		if(pStr[i] == '*' || (pStr[i] == '/' && (pStr[i + 1] == '*' || pStr[i + 1] == '/')))return true;
	}
	return false;
}

// void search_file(const std::string&cPath, const char *filename){
// 	//有路径找文件，有文件名找路径
// }
/*}}}*/
void search(const std::string&cPath, const std::string&filename, const std::string&lpstr, void(*fun)(const std::string&content, const std::string&lpstr, std::vector<std::string>&str)){
	std::string szPath;
	char *content = nullptr; 
	if('/' != cPath[cPath.length() - 1])
		szPath = cPath + "/" + filename;
	else
		szPath = cPath + filename;
	FILE *fp = fopen(szPath.c_str(), "rb");
	if(!fp){
		perror("read file error");
		printf("path is %s\n", szPath.c_str());
		return;
	}
	int size = getfilelen(fp);
	content = new char[size + 1];
	fread(content, size, 1, fp);
	content[size] = 0;
	fclose(fp);

	remove_comment(content);

	std::vector<std::string>str;
	fun(content, lpstr, str);
	int line;
	const char *lpStart = 0;//show line
	if(!str.empty()){
#ifdef __linux
		printf("\e[32m%s\e[0m\n", szPath.c_str());
#endif
#ifdef WIN32
		SetTextColor(FOREGROUND_GREEN);
		printf("%s\n", szPath);
		SetTextColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
	}
	int find_str_len = lpstr.length();
	int contentLineCount = linage(content);
	for(int i = 0; i < str.size(); i++){
		// if(!isCommit(str[i].c_str(), str[i].length())){//都已经移除注释内容了为什么还要检查？
		lpStart = strstr(content, str[i].c_str());
		line = contentLineCount - linage(lpStart) + 1;
#ifdef __linux
		str[i].insert(str[i].find(lpstr), "\e[31m");
		str[i].insert(str[i].find(lpstr) + find_str_len, "\e[0m");
#endif
		printf("%d:%s\n",line, str[i].c_str());
		// }
	}
	delete[]content;
}
bool isFun(const std::string&str, const char *fun_name){
	bool bIsFun = false;
	/*
		函数:返回类型 函数名(零个或多个参数,可能带回车符);
		void (*(*fun(...))(...))(...) ;


		//---------
		函数参数必须是带类型的，这里没有判断
	*/
	size_t funNameSize = strlen(fun_name);
	size_t funNamePos = str.find(fun_name);
	size_t c = str.find('(', funNamePos + funNameSize);
	size_t _c = str.find(')', funNamePos + funNameSize);
	//排除连基本的函数声明特征都没有的字符串(没有指定的函数名没有一对括号)
	if(std::string::npos != funNamePos && std::string::npos != c && std::string::npos != _c && std::string::npos == str.find("if (") && std::string::npos == str.find("return ")){
		//排除不可能出现在函数声明的字符
		size_t comma = str.find(',');
		size_t point = str.find('.');
		if((comma == std::string::npos || c < comma) && (std::string::npos == point || str[point + 1] == '.') && std::string::npos == str.find("#") && std::string::npos == str.find("->") && std::string::npos == str.find("typedef") && std::string::npos == str.find(':') && std::string::npos == str.find('!') && std::string::npos == str.find('}') && std::string::npos == str.find('{')){
			//等号有可能是默认参数。必须额外判断
			size_t p = str.find('=');
			if(std::string::npos == p || (p > c && p < _c)){//找到的括号一定在查找的字符串后面
				for (size_t i = 0; i < funNamePos - 1; ++i){//看看前面是否有返回类型
					if(str[i] != ' ' && str[i] != '\t'){
						bIsFun = true;
						break;
					}
				}
			}
		}
	}
	// char *buffer = new char[str_size + 1];
	// memcpy(buffer, lpstr, str_size);
	// buffer[str_size] = 0;
	// if(strchr(buffer, '(')){
	// 	char *p = strstr(buffer, fun_name);//不知道为什么，复制到buffer的内容居然不包含函数名。将直接跳过这样的内容
	// 	if(p){
	// 		for(p += strlen(fun_name); *p && *p != '('; ++p){
	// 			if(*p != ' ' && !isalpha(*p)){
	// 				bIsFun = false;
	// 				break;
	// 			}
	// 		}
	// 		if(*p == '(' && *(p + 1) != ')')bIsFun = true;
	// 	}
	// }
	// delete[]buffer;
	return bIsFun;
	//搜索效率
// 	char lpReg[100] = {0};
// 	sprintf(lpReg, "[^#/*].*[*&]? [*&]?%s.*[ ]?(.*)?[;]?[\r]?", fun_name);
// 	std::regex reg(lpReg);
// 	std::smatch result;
// 	std::string str(lpstr, str_size);
// 	return regex_match(str, result, reg);
}
//--前 ++后
/*{{{*/
const char *movepointer(const char *p, char ch, bool bfront){
	if (!p)return nullptr;
	if (bfront) {
		while (*p && *p != ch && p--);
	}
	else {
		p = strchr(p, ch);
	}
	return p;
}
/*}}}*/
void search_fun(const std::string&content, const std::string&lpstr, std::vector<std::string>&str){
	// std::smatch result;
	// char lpReg[100] = {0};
	const char *lpStart = content.c_str();
	// sprintf(lpReg, "[^#/*].*[*&]? [*&]?%s.*[ ]?(.*)?[\n]?.*[;]?[\r]?", lpstr.c_str());
	// sprintf(lpReg, "[^#/*].*[*&]? [*&]?%s.*[ ]?(.*[\n]?.*[\n]?.*[\n]?.*[\n]?.*[\n]?.*)[\n]?.*;", lpstr.c_str());//可能存在正则表达式匹配死循环的情况
	// std::regex reg(lpReg);
	while((lpStart = strstr(lpStart, lpstr.c_str()))){
// 		//---判断找到的字符串是否是函数
// //		printf("%.*s\n", 50, lpStart);
		lpStart = movepointer(lpStart, '\n', true);lpStart++;
		int lineSize = strcspn(lpStart, "\n");
		int len = strcspn(lpStart, ";");
		if(isFun(std::string(lpStart, len + 1), lpstr.c_str())){
			str.push_back(std::string(lpStart, len + 1));
		}
		// // sprintf(lpReg, "[^#/*].*[*&]? [*&]?%s.*[ ]?(.*)?[;]?[\r]?", lpstr.c_str());//匹配不到函数声明，而是一堆调用
		// // sprintf(lpReg, ".*%s.*", lpstr.c_str());//匹配不到函数声明，而是一堆调用
		// // sprintf(lpReg, "[^#/*].*[*&]? [*&]?%s.*[ ]?(.*)?[\r]?[;]?[\r]?", lpstr.c_str());//匹配不到函数声明，而是一堆调用
		// std::string regexStr(lpStart, len + 1);
		// // std::cout << regexStr << std::endl;
		// //试图手动过滤一些不匹配的字符串
		// std::size_t pos = regexStr.find('=', 0);//过滤左括号前有=号的字符串
		// // if(pos != std::string::npos){
		// // 	printf("h\n");
		// // }
		// if((std::string::npos ==  pos|| pos > regexStr.find('(', 0)) && std::string::npos == regexStr.find("return", 0)){
		// 	if(regex_match(regexStr, result, reg)){
		// 		std::string _str(lpStart, len + 1);
		// 		str.push_back(_str);
		// 	}
		// }
// 		if(*lpStart != '#'){
// 			if(memchr(lpStart, '\\', lineSize) || !memchr(lpStart, '(', lineSize)){
// 				lpStart += lineSize + 1;
// 				continue;
// 			}
// 			int len = strcspn(lpStart, ";");
// 			if(lpstr.find('(') != std::string::npos || isFun(lpStart, len + 1, lpstr.c_str())){
// 				std::string _str(lpStart, len + 1);
// 				str.push_back(_str);
// 			}
		// }
		/*
			加入查找的是strcpy，在以下字符串查找。因为分号比strcpy的位置还前，导致lpStart无法找到下一个strcpy，故而进入死循环。
			这也说明在strpy这一行前面有分号。可以说明查找的不是函数
			//一般而言，函数声明的分号一般都会在回车符结尾
			------
			\n  if ( _arg ) { this->var = new char[ strlen(_arg) + 1 ]; strcpy(this->var, _arg); }
		*/
		if(len < lineSize - 1){
			lpStart += lineSize;
			continue;
		}
		lpStart += len + 1;
	}
}
/*{{{*/
void search_macro(const std::string&content, const std::string&lpstr, std::vector<std::string>&str){
	const char *lpStart = content.c_str();
	char buffer[MAXBYTE] = {0};
	sprintf(buffer, "#define %s", lpstr);
	while((lpStart = strstr(lpStart, buffer))){
		int len = strcspn(lpStart, "\n");
		if('\\' == *(lpStart + len - 1)){
			do{
				len += strcspn(lpStart, "\n");
			}while('\\' == *(lpStart + len - 1));
		}
		std::string buff(lpStart, len);
		str.push_back(buff);
		lpStart += lpstr.length() + strlen("#define ");
	}
}
/*}}}*/
void search_struct(const std::string&content, const std::string&lpstr, std::vector<std::string>&str){
//just search struct name;no search struct alias name
	int count = content.length();
	char *Buff = new char[count + 1];
	memset(Buff, 0, count + 1);
	strcpy(Buff, content.c_str());
	char *p = nullptr;
	char *lpStart = Buff;
	char buffer[MAXBYTE] = {0};
	sprintf(buffer, "%s %s", searchStructString.c_str(), lpstr.c_str());
	while((lpStart = strstr(lpStart, buffer))){
		lpStart -= strlen("typedef ");
		if(memcmp(lpStart, "typedef ", strlen("typedef")))lpStart += strlen("typedef ");
		p = strchr(lpStart, '\n');
		if(p && memchr(lpStart, '{', strcspn(lpStart, "\n")) && ';' != *(p - 1)){
			int count = 1;
			p = strchr(lpStart, '{');
			do{
				p++;
					if(*p == '{')count++;
				if(*p == '}')count--;
				if(!p)break;
			}while(count);
			p = strchr(p, '\n');
		}
		if(p){
			*p = 0;
			std::string buff(lpStart);
			str.push_back(buff);
		}
		lpStart += lpstr.length() + searchStructString.length() + strlen("typedef");
	}
	delete[]Buff;
}
void search_type_define(const std::string&content, const std::string&lpstr, std::vector<std::string>&str){
	int count = content.length();
	char *Buff = new char[count + 1];
	memset(Buff, 0, count + 1);
	strcpy(Buff, content.c_str());
	char *lpStart = Buff;
	char buffer[MAXBYTE] = {0};
	sprintf(buffer, "typedef %s", lpstr.c_str());
	while((lpStart = strstr(lpStart, buffer))){
		char *p = strchr(lpStart, '\n');
		if(p){
			*p = 0;
			std::string buff(lpStart);
			str.push_back(buff);
		}
		lpStart += lpstr.length() + strlen("typedef ");
	}
	delete[]Buff;
}
#ifdef WIN32
void SetTextColor(WORD color){
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hout, color);
}
#endif
bool get_val_in_line(int argc, char *argv[], const std::string&lpsstr, std::string&lpstr, int32_t start){
	int index = get_index_in_line(argc, argv, lpsstr, start);
	if(INVALID_VAL != index && argv[index + 1]){
		lpstr = argv[index + 1];
		return true;
	}
	return false;
}
void help(){
	printf("format:option string string [option][string]...\n");
	printf("example:query -f strcpy strcat -s VkDeviceC -f vkCmdDraw -s VkDeviceCreateInfo -sf string.h vulkan_core.h\n");
	printf("-nd表示不在该路及内搜索\n");
	printf("-nsf表示不在该文件内搜索\n");
	printf("option:\n");
	printf("\t'-e' indicate search enum\n");
	printf("\t'-e' indicate search union\n");
	printf("\t'-c' indicate search class\n");
	printf("\t'-m' indicate search macro\n");
	printf("\t'-f' indicate search function\n");
	printf("\t'-s' indicate search structure\n");
	printf("\t'-d' indicate search directory\n");
	printf("\t'-ns' indicate search namespace\n");
	printf("\t'-t' indicate search type define\n");
	printf("\t'-sf' indicate search in that file;\n");
}
bool isInvalid(int argc, char *argv[], const std::vector<std::string>&option){
	int ioption = 0;
	int optCount = 0;
	if(argc < 3)return true;
	for(int j = 0; j < option.size(); j++){
		if(INVALID_VAL == get_index_in_line(argc, argv, option[j]))++optCount;
		for(int i = 1; i < argc; i++){
			if(option[j] == argv[i]){
				ioption++;
				break;
			}
		}
	}
	//说明不是只有传一个 选项
	return (ioption == argc - 1) || optCount == option.size();
}
int get_index_in_line(int argc, char *argv[], const std::string&str, int32_t start){
	for(int i = start; i < argc; i++){
		if(argv[i] == str){
			return i;
		}
	}
	return INVALID_VAL;
}
int getfilelen(FILE *fp){
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}
void remove_comment(char *content){
	char *temp;
	char *lpStart = content;
	while((lpStart = strstr(lpStart, "//"))){
		char *p = strchr(lpStart, '\n');
		if(p){
			// *p = 0;
			// movepointer();
			// if(strchr(lpStart, '\\')){//如果注释后面带一个\符号的话，说明下一行也是注释。对下一行也需要这样检查

			// }
			p += 1;
			int len = strlen(p);
			temp = new char[len + 1];
			// memset(temp, 0, len + 1);
			strcpy(temp, p);
			strcpy(lpStart, temp);
			lpStart[len] = 0;
			delete[]temp;
		}
		lpStart++;
	}
	lpStart = content;
	while((lpStart = strstr(lpStart, "/*"))){
		char *p = strstr(lpStart, "*/");
		if(p){
			p += 2;
			int len = strlen(p);
			temp = new char[len + 1];
			// memset(temp, 0, len + 1);
			strcpy(temp, p);
			strcpy(lpStart, temp);
			lpStart[len] = 0;
			delete[]temp;
		}
		lpStart++;
	}
}
int linage(const std::string&content){
	int line = 1;
	const char *lpStart = content.c_str();
	while((lpStart = strchr(lpStart, '\n')) && ++line && ++lpStart);
	return line;
}
/*
char *strrpc(char *str,char *oldstr,char *newstr){
	int len = strlen(str);
	char *bstr = new char[len + 1];//转换缓冲区
	memset(bstr,0, len + 1);
	for(int i = 0;i < strlen(str);i++){
		if(!strncmp(str+i,oldstr,strlen(oldstr))){//查找目标字符串
			strcat(bstr,newstr);
			i += strlen(oldstr) - 1;
		}else{
			strncat(bstr,str + i,1);//保存一字节进缓冲区
	    }
    	}
	strcpy(str,bstr);
	delete[]bstr;
	return str;
}
*/
bool isOption(int argc, char *argv[], int index){
	const std::vector<std::string> option = { FUNCTION_OPTION, MACRO_OPTION,  STRUCTURE_OPTION, TYPEDEF_OPTION, UNION_OPTION, ENUM_OPTION, CLASS_OPTION, NAMESPACE_OPTION,  PATH_OPTION, FILE_OPTION, NO_SEARCH_FILE_OPTION };
	for(int i = 0; i < option.size(); ++i){
		if(argv[index] == option[i]){
			return true;
		}
	}
	return false;
}
void get_option_val(int argc, char *argv[], const std::string&opt, std::vector<std::string>&out){
	int offset = 1;
	int index = INVALID_VAL;
	while(INVALID_VAL != (index = get_index_in_line(argc, argv, opt, offset))){
		++index;
		while(argv[index] && !isOption(argc, argv, index)){
			out.push_back(argv[index]);
			++index;
		}
		offset = index;
	}
}

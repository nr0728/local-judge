#include<bits/stdc++.h>
#include<Windows.h>
#include<psapi.h>
#include<thread>
#define rep(i,a,b) for(int i(a);i<=(b);++i)
#define req(i,a,b) for(int i(a);i>=(b);--i)
using namespace std;
map<string,string> Arg;
vector<string> ls(string path)
{
	//Get ALL filenames in the folder
	//just execute dir and parse the output
	vector<string> res;
	string command="dir /b "+path;
	FILE* pipe=_popen(command.c_str(),"r");
	if(!pipe) return res;
	char buffer[128];
	while(!feof(pipe))
	{
		if(fgets(buffer,128,pipe)!=NULL)
		{
			string filename=buffer;
			if(filename.back()=='\n') filename.pop_back();
			res.push_back(filename);
		}
	}
	_pclose(pipe);
	return res;
}
auto sorted(auto a)
{
	sort(a.begin(),a.end());
	return a;
}
int parseInt(string a)
{
	int res=0;
	for(auto i:a) res=res*10+i-'0';
	return res;
}
map<int,string> specialMapping;
int mappingExtension(string a)
{
	// map the extension to a number
	if(a==".in") return 0;
	if(a==".out") return 1;
	if(a==".ans") return 2;
	int hash_value=0;
	for(auto i:a) hash_value=(hash_value*31+i)%1000000007;
	specialMapping[hash_value]=a;
	return hash_value;
}
string parseExtension(int a)
{
	// map the number back to the extension
	if(a==0) return ".in";
	if(a==1) return ".out";
	if(a==2) return ".ans";
	return specialMapping[a];
}
auto sort_file(auto a)
{
	// for each string in a, if the string can be seperated into [string][number][extension], sort first by string, then by number, then by extension
	// if the string cannot be seperated, sort by the whole string, and put the ones cannot be seperated at the end of the list
	vector<string> res,cannot;
	vector<tuple<string,int,int>> b;
	// [string][number][extension]
	for(auto i:a)
	{
		string s="",n="",e="";
		int j=0;
		for(;j<(int)i.size()&&!isdigit(i[j]);++j) s+=i[j];
		for(;j<(int)i.size()&&isdigit(i[j]);++j) n+=i[j];
		for(;j<(int)i.size();++j) e+=i[j];
		if(n.empty()) cannot.push_back(i);
		else b.push_back({s,parseInt(n),mappingExtension(e)});
	}
	sort(b.begin(),b.end());
	for(auto i:b) res.push_back(get<0>(i)+to_string(get<1>(i))+parseExtension(get<2>(i)));
	for(auto i:cannot) res.push_back(i);
	return res;
}
mt19937 rnd(chrono::steady_clock::now().time_since_epoch().count());
string random_filename()
{
	// generate a random filename
	string res="temp\\";
	rep(i,1,10) res+=char(rnd()%26+'a')-(rnd()&1?32:0);
	res+=".cpp";
	return res;
}
// Helper function to trim trailing whitespace (spaces, tabs, carriage returns)
static void trimTrailingWhitespace(std::string &line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
        line.pop_back();
    }
}

// Reads all lines from a file into a vector, trimming trailing whitespace.
// Later, we'll remove trailing lines that contain only invisible characters.
static std::vector<std::string> readAndProcessLines(const std::string &filename) {
    std::ifstream file(filename);
    std::vector<std::string> lines;
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Remove trailing whitespace from the line
        trimTrailingWhitespace(line);
        // Add the line (it may be empty if it had only whitespace before) to lines
        lines.push_back(line);
    }
    file.close();
    
    // Remove empty (invisible) lines from the end of the vector
    // Here "empty" means after trimming, it has no visible characters left.
    while (!lines.empty()) {
        // Check if line contains only invisible characters (should be empty now after trimming)
        if (!lines.back().empty()) {
            break;
        }
        lines.pop_back();
    }
    
    return lines;
}

bool compareFiles(const std::string &fileA, const std::string &fileB) {
    // Read processed lines from both files
    std::vector<std::string> linesA = readAndProcessLines(fileA);
    std::vector<std::string> linesB = readAndProcessLines(fileB);
    
    // If different number of meaningful lines, files differ
    if (linesA.size() != linesB.size()) {
        return false;
    }
    
    // Compare line by line
    for (size_t i = 0; i < linesA.size(); ++i) {
        if (linesA[i] != linesB[i]) {
            return false;
        }
    }
    
    return true;
}

// 这些常量可根据需要自行修改或从外部参数中读取
static DWORD TIMEOUT = 10000;         // 毫秒级别的超时
static SIZE_T MEMORY_LIMIT = 256 * 1024 * 1024; // 256MB
static const DWORD POLL_INTERVAL = 5; // 进程轮询周期 (ms)

// 定义评测结果枚举，与题中描述的判定常量对应
enum {
    ACCEPTED = 0,
    WRONG_ANSWER = 1,
    TIME_LIMIT_EXCEEDED = 2,
    MEMORY_LIMIT_EXCEEDED = 3,
    RUNTIME_ERROR = 4,
    OUTPUT_LIMIT_EXCEEDED = 5,
    SYSTEM_ERROR = 6
};

struct JudgeInfo {
    int result;
    SIZE_T memory;
    DWORD time;
    JudgeInfo() : result(SYSTEM_ERROR), memory(0), time(0) {}
};

SIZE_T GetProcessMemoryUsage(HANDLE hProcess) {
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
	return pmc.PrivateUsage;
}

// judge function: runs the compiled binary, limits time (TL) and memory (ML),
// kills the process if limits are exceeded, and compares output for AC/WA.
// It logs the result in evaluator_log and outputs: {time(ms), memory(kb), VERDICT}
// VERDICT includes: AC, WA, TLE, MLE, OLE, UKE (Unknown Error)
string problem_id,submission_id,input_file_name,output_file_name,time_limit,memory_limit,checker_file;

// 新的核心评测函数，用于替换原先 judge 函数内的过程
JudgeInfo judgeProgram(const string &filename,
                       const string &input,
                       const string &output,
                       const string &answer)
{
    JudgeInfo result;
    SECURITY_ATTRIBUTES sa, sb, sc;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    sb.nLength = sizeof(SECURITY_ATTRIBUTES);
    sb.lpSecurityDescriptor = NULL;
    sb.bInheritHandle = TRUE;
    sc.nLength = sizeof(SECURITY_ATTRIBUTES);
    sc.lpSecurityDescriptor = NULL;
    sc.bInheritHandle = TRUE;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    si.dwFlags |= STARTF_USESTDHANDLES;

    // 准备重定向输入输出
    HANDLE hInput = CreateFile(input.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hOutput = CreateFile(output.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, &sb, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    si.hStdInput = hInput;
    si.hStdOutput = hOutput;
    si.hStdError = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sc, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // 创建进程
    bool success = CreateProcess(
        NULL,
        (LPSTR)filename.c_str(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (success) {
        // 如果进程创建成功，则轮询检查是否超时、内存超限等
        DWORD startTime = GetTickCount64();
        while (true) {
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            if (exitCode != STILL_ACTIVE) break;

            DWORD currentTime = GetTickCount64();
            if (currentTime - startTime >= TIMEOUT) {
                result.result = TIME_LIMIT_EXCEEDED;
                TerminateProcess(pi.hProcess, 1);
                break;
            }
            SIZE_T memoryUsage = GetProcessMemoryUsage(pi.hProcess);
            // 记录最大占用
            result.memory = max(result.memory, memoryUsage);
            if (memoryUsage > MEMORY_LIMIT) {
                result.result = MEMORY_LIMIT_EXCEEDED;
                TerminateProcess(pi.hProcess, 1);
                break;
            }
            Sleep(POLL_INTERVAL);
        }

        DWORD currentTime = GetTickCount64();
        result.time = currentTime - startTime;

        // 获取进程退出码
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        if (exitCode != 0 && result.result == SYSTEM_ERROR) {
            // 如果尚未设置为 TLE/MLE，但进程退出码非 0
            result.result = RUNTIME_ERROR;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hInput);
        CloseHandle(hOutput);
        Sleep(100); // 给系统一点缓冲时间

        // 如果到这一步都没设置，则说明没有 TLE、MLE、RE
        if (result.result == SYSTEM_ERROR) {
            // 根据实际需求，调用对应对比函数
            bool differ = compareFiles(output, answer);
            // 注意：本示例里 diff_* 函数返回 true 代表“相同”，
            // 你可根据自己的实现对逻辑进行调整
            if (differ) {
                result.result = ACCEPTED;
            } else {
                result.result = WRONG_ANSWER;
            }
        }
    } else {
        // CreateProcess 失败
        result.result = SYSTEM_ERROR;
    }
    return result;
}

// 替换 judge 函数中的评测流程，调用新函数 judgeProgram
void judge(const std::string &input_file,
           const std::string &output_file,
           const std::string &executable,
           const std::string &file_io,
           const std::string &time_limit_str,
           const std::string &memory_limit_str,
           const std::string &special_judge,
           const std::string &checker_file,
           std::ofstream &evaluator_log)
{
    // 若需要，将 time_limit 与 memory_limit 转成数值并赋给 TIMEOUT / MEMORY_LIMIT
    // 这里简单处理为秒 -> 毫秒，MB -> 字节
    // stoi/ stoll 转换可能需要考虑异常情况，简化处理:
    TIMEOUT = (DWORD)(std::stoll(time_limit_str));
    MEMORY_LIMIT = (SIZE_T)(std::stoll(memory_limit_str) * 1024);

    // 如果 file_io == "1"，程序会自己使用 input_file_name / output_file_name 进行IO
    // 否则我们重定向到 temp\output.txt
    // 使得 judgeProgram 调用时： 
    //   文件名 = executable (可执行文件)
    //   input  = input_file
    //   output = (file_io == "1" ? output_file_name : "temp\\output.txt")
    //   answer = output_file (期望答案文件)
    string user_output_file = (file_io == "1") ? output_file_name : "temp\\output.txt";

    JudgeInfo info = judgeProgram(
        executable,        // 可执行文件
        input_file,        // 输入文件
        user_output_file,  // 输出文件
        output_file        // 答案文件
    );

    // 额外进行 OLE 检查 (比如 100MB)
    // 仅当程序正常退出且不是 TLE/MLE/RE/SYSTEM_ERROR 时才检查
    if (info.result == ACCEPTED || info.result == WRONG_ANSWER) {
        std::ifstream outCheck(user_output_file, std::ios::binary | std::ios::ate);
        if (outCheck.is_open()) {
            std::streamsize size = outCheck.tellg();
            if (size > 100LL * 1024 * 1024) {
                info.result = OUTPUT_LIMIT_EXCEEDED;
            }
        }
    }

    // 映射回原判题系统需要的字符串
    // AC WA TLE MLE OLE RE UKE
    string verdict = "UKE";
    switch(info.result) {
        case ACCEPTED: verdict = "AC"; break;
        case WRONG_ANSWER: verdict = "WA"; break;
        case TIME_LIMIT_EXCEEDED: verdict = "TLE"; break;
        case MEMORY_LIMIT_EXCEEDED: verdict = "MLE"; break;
        case OUTPUT_LIMIT_EXCEEDED: verdict = "OLE"; break;
        case RUNTIME_ERROR: verdict = "RE"; break;
        default: verdict = "UKE"; break;
    }

    // 将评测结果记录到日志
    // 时间以毫秒计；内存以 KB 计
    evaluator_log << "TIME " << info.time
                  << " MEMORY " << (info.memory / 1024)
                  << " VERDICT " << verdict << std::endl;

    // 清理
    string executablename = executable.substr(executable.find_last_of("\\")+1);
    system(("taskkill /f /im "+executablename+" >nul 2>nul").c_str());
    system("del temp\\output.txt >nul 2>nul");
}

signed main(int argc,char* argv[])
{
	// evaluator --problem [ID] --submission-id [ID] --file-io [0/1] --input-file [stdin/input_filename] --output-file [stdout/output_filename] --time-limit [time_limit] --memory-limit [memory_limit] --special-judge [0/1] --checker-file [none/special_judge_filename]
	// file is stored in \\submissions\\[ID].cpp
	// problem data is stored in \\problems\\data\\[ID]
	// The folder contains matched input and output files (e.g. 1.in, 1.out; i.in, i.out)
	// Run on Windows
	if(argc!=19) return cerr<<"Invalid arguments"<<endl,1;
	for(int i=1;i<argc;i+=2)
		Arg[argv[i]]=argv[i+1];
	problem_id=Arg["--problem"];
	submission_id=Arg["--submission-id"];
	input_file_name=Arg["--input-file"];
	output_file_name=Arg["--output-file"];
	time_limit=Arg["--time-limit"];
	memory_limit=Arg["--memory-limit"];
	checker_file=Arg["--checker-file"];
	vector<pair<string,string>> files;
	// Get all .in/.out in the folder ./problems/data/[ID]/ and match them into pairs
	string path="problems\\data\\"+problem_id+"\\";
	vector<string> all=sort_file(ls(path));
	for(auto i:all)
	{
		if(i.find(".in")!=string::npos)
		{
			string j=i.substr(0,i.size()-3)+".out";
			if(find(all.begin(),all.end(),j)!=all.end()) files.push_back({i,j});
			else
			{
				string k=i.substr(0,i.size()-3)+".ans";
				if(find(all.begin(),all.end(),k)!=all.end()) files.push_back({i,k});
			}
		}
	}
	// output the judgement log to log\\evaluator-[submission_id].log
	ofstream evaluator_log("log\\evaluator-"+submission_id+".log");
	for(auto i:files) evaluator_log<<"FILE "<<i.first<<" "<<i.second<<endl;
	// user code is \\submissions\\[ID].cpp
	// copy to \\temp\[random-filename].cpp
	string random_file=random_filename(),random_exe=random_file.substr(0,random_file.size()-4)+".exe";
	string command="copy submissions\\"+submission_id+".cpp "+random_file;
	system(command.c_str());
	string compile_command="g++ "+random_file+" -o "+random_exe+" -O2 -std=c++17 -DONLINE_JUDGE 2> log\\compile-"+submission_id+".log";
	if(system(compile_command.c_str()))
	{
		evaluator_log<<"TIME 0 MEMORY 0 VERDICT CE"<<endl;
		return 1;
	}
	// run the user code on each pair of input/output files
	for(auto [i,o]:files)
	{
		string input_file=path+i,output_file=path+o;
		judge(input_file,output_file,random_exe,Arg["--file-io"],time_limit,memory_limit,Arg["--special-judge"],checker_file,evaluator_log);
	}
	return 0;
}

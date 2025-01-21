#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <tuple>
#include <filesystem>
#include <Windows.h>
#include <psapi.h>

enum class JudgeResult {
    ACCEPTED,
    WRONG_ANSWER,
    TIME_LIMIT_EXCEEDED,
    MEMORY_LIMIT_EXCEEDED,
    RUNTIME_ERROR,
    OUTPUT_LIMIT_EXCEEDED,
    SYSTEM_ERROR
};

struct JudgeInfo {
    JudgeResult result;
    SIZE_T memory;
    DWORD time;
    JudgeInfo() : result(JudgeResult::SYSTEM_ERROR), memory(0), time(0) {}
};

class Utils {
public:
    static std::vector<std::string> executeCommand(const std::string& command) {
        std::vector<std::string> result;
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) return result;
        char buffer[128];
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL) {
                std::string line = buffer;
                if (line.back() == '\n') line.pop_back();
                result.push_back(line);
            }
        }
        _pclose(pipe);
        return result;
    }

    static int parseInt(const std::string& str) {
        int res = 0;
        for (char c : str) {
            res = res * 10 + (c - '0');
        }
        return res;
    }

    static void trimTrailingWhitespace(std::string& line) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }
    }

    static std::vector<std::string> readAndProcessLines(const std::string& filename) {
        std::ifstream file(filename);
        std::vector<std::string> lines;
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return lines;
        }
        std::string line;
        while (std::getline(file, line)) {
            trimTrailingWhitespace(line);
            lines.push_back(line);
        }
        file.close();
        while (!lines.empty() && lines.back().empty()) {
            lines.pop_back();
        }
        return lines;
    }

    static std::string generateRandomFilename() {
        static std::mt19937 rnd(std::chrono::steady_clock::now().time_since_epoch().count());
        std::string res = "temp\\";
        for (int i = 0; i < 10; ++i) {
            res += char(rnd() % 26 + 'a') - (rnd() & 1 ? 32 : 0);
        }
        res += ".cpp";
        return res;
    }

    static int getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

class FileNameSorter {
private:
    std::map<int, std::string> specialMapping;
    int mapExtension(const std::string& ext) {
        if (ext == ".in") return 0;
        if (ext == ".out") return 1;
        if (ext == ".ans") return 2;
        int hashValue = 0;
        for (char c : ext) {
            hashValue = (hashValue * 31 + c) % 1000000007;
        }
        specialMapping[hashValue] = ext;
        return hashValue;
    }

    // 将数字映射回文件扩展名
    std::string parseExtension(int id) {
        if (id == 0) return ".in";
        if (id == 1) return ".out";
        if (id == 2) return ".ans";
        return specialMapping[id];
    }
public:
    // 对文件名列表进行排序
    std::vector<std::string> sortFiles(const std::vector<std::string>& files) {
        std::vector<std::string> result, cannot;
        std::vector<std::tuple<std::string, int, int>> sortData;
        for (const std::string& file : files) {
            std::string s, n, e;
            int j = 0;
            for (; j < (int)file.size() && !isdigit(file[j]); ++j) s += file[j];
            for (; j < (int)file.size() && isdigit(file[j]); ++j) n += file[j];
            for (; j < (int)file.size(); ++j) e += file[j];
            if (n.empty()) {
                cannot.push_back(file);
            } else {
                sortData.emplace_back(s, Utils::parseInt(n), mapExtension(e));
            }
        }
        std::sort(sortData.begin(), sortData.end());
        for (const auto& data : sortData) {
            result.push_back(std::get<0>(data) + std::to_string(std::get<1>(data)) + parseExtension(std::get<2>(data)));
        }
        result.insert(result.end(), cannot.begin(), cannot.end());
        return result;
    }
};

class FileComparator {
public:
    static bool compareFiles(const std::string& fileA, const std::string& fileB) {
        std::vector<std::string> linesA = Utils::readAndProcessLines(fileA);
        std::vector<std::string> linesB = Utils::readAndProcessLines(fileB);
        if (linesA.size() != linesB.size()) return false;
        for (size_t i = 0; i < linesA.size(); ++i) {
            if (linesA[i] != linesB[i]) return false;
        }
        return true;
    }
};

class JudgeInterface {
public:
    virtual JudgeInfo judge(const std::string& executable, const std::string& input, const std::string& output, const std::string& answer, const std::string& spj, int testid) = 0;
    virtual ~JudgeInterface() = default;
};

class WindowsJudge : public JudgeInterface {
private:
    DWORD timeout;
    SIZE_T memoryLimit;
    const DWORD pollInterval = 5;
    std::string submissionId;
    std::ofstream extlog;
public:
    WindowsJudge(DWORD timeout, SIZE_T memoryLimit, std::map<std::string, std::string>& args) :
        timeout(timeout),
        memoryLimit(memoryLimit),
        submissionId(args["--submission-id"]),
        extlog("log\\extra-" + submissionId + ".log") {};
    JudgeInfo judge(const std::string& filename, const std::string& input, const std::string& output, const std::string& answer, const std::string& spj, int testid) override {
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
        HANDLE hInput = CreateFile(input.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        HANDLE hOutput = CreateFile(output.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, &sb, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        si.hStdInput = hInput;
        si.hStdOutput = hOutput;
        si.hStdError = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sc, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        bool success = CreateProcess(NULL, (LPSTR)filename.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        if (success) {
            DWORD startTime = GetTickCount64();
            while (true) {
                DWORD exitCode;
                GetExitCodeProcess(pi.hProcess, &exitCode);
                if (exitCode != STILL_ACTIVE) break;
                DWORD currentTime = GetTickCount64();
                if (currentTime - startTime >= timeout) {
                    result.result = JudgeResult::TIME_LIMIT_EXCEEDED;
                    TerminateProcess(pi.hProcess, 1);
                    break;
                }
                SIZE_T memoryUsage = getProcessMemoryUsage(pi.hProcess);
                result.memory = std::max(result.memory, memoryUsage);
                if (memoryUsage > memoryLimit) {
                    result.result = JudgeResult::MEMORY_LIMIT_EXCEEDED;
                    TerminateProcess(pi.hProcess, 1);
                    break;
                }
                Sleep(pollInterval);
            }
            DWORD currentTime = GetTickCount64();
            result.time = currentTime - startTime;
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            if (exitCode != 0 && result.result == JudgeResult::SYSTEM_ERROR) {
                result.result = JudgeResult::RUNTIME_ERROR;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hInput);
            CloseHandle(hOutput);
            Sleep(100);
            if (result.result == JudgeResult::SYSTEM_ERROR) {
                bool differ = (spj == "none") ? FileComparator::compareFiles(output, answer) : customJudge(input, output, answer, testid);
                result.result = differ ? JudgeResult::ACCEPTED : JudgeResult::WRONG_ANSWER;
            }
        } else {
            result.result = JudgeResult::SYSTEM_ERROR;
        }
        return result;
    }
private:
    SIZE_T getProcessMemoryUsage(HANDLE hProcess) {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
        return pmc.PrivateUsage;
    }
    bool customJudge(const std::string& input, const std::string& output, const std::string& answer, int testid) {
        std::string checkerOutput = "temp\\checker.exe";
        std::string command = checkerOutput + " " + input + " " + output + " " + answer + " > temp\\checker_output.txt 2>&1";
        if (system(command.c_str())) {
            std::ifstream fin("temp\\checker_output.txt");
            extlog << "On Test " << testid << std::endl;
            std::string line;
            while (getline(fin, line)) extlog << line << std::endl;
            return false;
        }
        std::ifstream fin("temp\\checker_output.txt");
        extlog << "On Test " << testid << std::endl;
        std::string line;
        while (getline(fin, line)) extlog << line << std::endl;
        return true;
    }
};

class JudgeManager {
private:
    std::string problemId;
    std::string submissionId;
    std::string inputFileName;
    std::string outputFileName;
    std::string timeLimit;
    std::string memoryLimit;
    std::string checkerFile;
    std::string fileIO;
    std::ofstream evaluatorLog;
    FileNameSorter fileNameSorter;
    JudgeInterface* judgeInstance;
public:
    JudgeManager(std::map<std::string, std::string>& args) :
        problemId(args["--problem"]),
        submissionId(args["--submission-id"]),
        inputFileName(args["--input-file"]),
        outputFileName(args["--output-file"]),
        timeLimit(args["--time-limit"]),
        memoryLimit(args["--memory-limit"]),
        checkerFile(args["--checker-file"]),
        fileIO(args["--file-io"]),
        evaluatorLog("log\\evaluator-" + submissionId + ".log")
    {
        DWORD timeout = (DWORD)(std::stoll(timeLimit));
        SIZE_T memoryLimitBytes = (SIZE_T)(std::stoll(memoryLimit) * 1024);
        judgeInstance = new WindowsJudge(timeout, memoryLimitBytes, args);
    }
    ~JudgeManager() {
        delete judgeInstance;
    }
    void run() {
        std::vector<std::pair<std::string, std::string>> files = getTestFiles();
        for (const auto& filePair : files) {
            evaluatorLog << "FILE " << filePair.first << " " << filePair.second << std::endl;
        }
        std::string randomFile = Utils::generateRandomFilename();
        std::string randomExe = randomFile.substr(0, randomFile.size() - 4) + ".exe";
        std::string copyCommand = "copy submissions\\" + submissionId + ".cpp " + randomFile + ">nul 2>nul";
        system(copyCommand.c_str());
        std::string compileCommand = "g++ " + randomFile + " -o " + randomExe + " -O2 -std=c++17 -DONLINE_JUDGE 2> log\\compile-" + submissionId + ".log";
        std::cerr << "Compiling Program..." << std::endl;
        if (system(compileCommand.c_str())) {
            for (int _ = 0; _ < (int)files.size(); ++_) {
                evaluatorLog << "TIME 0 MEMORY 0 VERDICT CE" << std::endl;
            }
            std::cerr << "Compilation Error:" << std::endl;
            std::ifstream fin("log\\compile-" + submissionId + ".log");
            std::string line;
            while (getline(fin, line)) std::cerr << line << std::endl;
            return;
        }
        std::cerr << "Compilation Successful." << std::endl;
        if (checkerFile != "none") {
            std::string checkerFilename = "problems\\data\\" + problemId + "\\" + checkerFile;
            std::string checkerOutput = "temp\\checker.exe";
            std::string command = "g++ -O2 -std=c++17 -o " + checkerOutput + " " + checkerFilename;
            std::cerr << "Compiling Checker..." << std::endl;
            if (system(command.c_str())) {
                for (int _ = 0; _ < (int)files.size(); ++_) {
                    evaluatorLog << "TIME 0 MEMORY 0 VERDICT UKE" << std::endl;
                }
                std::cerr << "Checker Compilation Error." << std::endl;
                return;
            }
            std::cerr << "Checker Compilation Successful." << std::endl;
        }
        int testId = 0;
        for (const auto& filePair : files) {
            std::string inputFile = "problems\\data\\" + problemId + "\\" + filePair.first;
            std::string outputFile = "problems\\data\\" + problemId + "\\" + filePair.second;
            std::cerr << "Testcase (<font face=\"Consolas\">" << inputFile.substr(inputFile.find_last_of("\\") + 1)
                   << "</font>, <font face=\"Consolas\">" << outputFile.substr(outputFile.find_last_of("\\") + 1)
                << "</font>)" << ": \n\t" << std::flush;
            judgeTestcase(inputFile, outputFile, randomExe, ++testId);
        }
    }
private:
    std::vector<std::pair<std::string, std::string>> getTestFiles() {
        std::vector<std::pair<std::string, std::string>> files;
        std::string path = "problems\\data\\" + problemId + "\\";
        std::vector<std::string> allFiles = fileNameSorter.sortFiles(Utils::executeCommand("dir /b " + path));
        for (const auto& file : allFiles) {
            if (file.find(".in") != std::string::npos) {
                std::string outFile = file.substr(0, file.size() - 3) + ".out";
                if (std::find(allFiles.begin(), allFiles.end(), outFile) != allFiles.end()) {
                    files.push_back({file, outFile});
                } else {
                    std::string ansFile = file.substr(0, file.size() - 3) + ".ans";
                    if (std::find(allFiles.begin(), allFiles.end(), ansFile) != allFiles.end()) {
                        files.push_back({file, ansFile});
                    }
                }
            }
        }
        return files;
    }
    std::map<std::string, std::string> verdictColor = {
        {"AC", "#52C41A"},
        {"WA", "#E74C3C"},
        {"TLE", "#052242"},
        {"MLE", "#052242"},
        {"OLE", "#052242"},
        {"CE", "#FADB14"},
        {"UKE", "#0E1D69"}
    };
    std::string convertColor(double p) {
        if (p < 0.0) p = 0.0;
        if (p > 1.0) p = 1.0;
        int red = static_cast<int>(std::round(p * 255));
        int green = static_cast<int>(std::round((1.0 - p) * 255));
        int blue = 0;
        std::stringstream ss;
        ss << "#"
            << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << red
            << std::setw(2) << std::setfill('0') << green
            << std::setw(2) << std::setfill('0') << blue;
        return ss.str();
    }
    std::string cerrWithColor(const std::string message, const std::string color) {
        return "<b> <font color=\"" + color + "\">" + message + "</font> </b>";
    }
    std::string convertToMB(const auto& memory) {
        double mem = memory;
        mem /= 1048576.0;
        return std::__cxx11::to_string(mem);
    }
    void judgeTestcase(const std::string& inputFile, const std::string& outputFile, const std::string& executable, int testId) {
        std::string userInputFile = (fileIO == "1") ? inputFileName : "temp\\input.txt";
        std::string userOutputFile = (fileIO == "1") ? outputFileName : "temp\\output.txt";
        if (fileIO == "1") {
            system(("copy " + inputFile + " " + userInputFile + " >nul 2>nul").c_str());
        }
        JudgeInfo info = judgeInstance->judge(executable, userInputFile, userOutputFile, outputFile, checkerFile, testId);
        if (info.result == JudgeResult::ACCEPTED || info.result == JudgeResult::WRONG_ANSWER) {
            std::ifstream outCheck(userOutputFile, std::ios::binary | std::ios::ate);
            if (outCheck.is_open()) {
                std::streamsize size = outCheck.tellg();
                if (size > 100LL * 1024 * 1024) info.result = JudgeResult::OUTPUT_LIMIT_EXCEEDED;
            }
        }
        system(("del " + userInputFile + " >nul 2>nul").c_str());
        system(("del " + userOutputFile + " >nul 2>nul").c_str());
        std::string verdict = "UKE";
        switch (info.result) {
            case JudgeResult::ACCEPTED: verdict = "AC"; break;
            case JudgeResult::WRONG_ANSWER: verdict = "WA"; break;
            case JudgeResult::TIME_LIMIT_EXCEEDED: verdict = "TLE"; break;
            case JudgeResult::MEMORY_LIMIT_EXCEEDED: verdict = "MLE"; break;
            case JudgeResult::OUTPUT_LIMIT_EXCEEDED: verdict = "OLE"; break;
            case JudgeResult::RUNTIME_ERROR: verdict = "RE"; break;
            default: verdict = "UKE"; break;
        }
        evaluatorLog << "TIME " << info.time << " MEMORY " << (info.memory / 1024) << " VERDICT " << verdict << std::endl;
        std::string cerrMessage = "";
        cerrMessage += cerrWithColor(verdict, verdictColor[verdict]);
        cerrMessage += " (Time: ";
        cerrMessage += cerrWithColor(std::__cxx11::to_string(info.time), convertColor(info.time * 1.0 / std::stoll(timeLimit)));
        cerrMessage += " ms, Memory: ";
        cerrMessage += cerrWithColor(convertToMB(info.memory), convertColor(info.memory / 1024.0 / std::stoll(memoryLimit)));
        cerrMessage += " MB)";
        std::cerr << cerrMessage << std::endl;
        std::string executableName = executable.substr(executable.find_last_of("\\") + 1);
        system(("taskkill /f /im " + executableName + " >nul 2>nul").c_str());
        system("del temp\\output.txt >nul 2>nul");
    }
};

int main(int argc, char* argv[]) {
    if (argc != 19) {
        std::cerr << "Invalid arguments." << std::endl;
        std::cerr << "Usage: evaluator --problem [ID] --submission-id [ID] --file-io [0/1] --input-file [stdin/input_filename] --output-file [stdout/output_filename] --time-limit [time_limit] --memory-limit [memory_limit] --special-judge [0/1] --checker-file [none/special_judge_filename]" << std::endl;
        return 1;
    }
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; i += 2) {
        args[argv[i]] = argv[i + 1];
    }
    JudgeManager judgeManager(args);
    judgeManager.run();
    std::cerr << "Timestamp: " << Utils::getCurrentTimestamp() << std::endl;
    return 0;
}

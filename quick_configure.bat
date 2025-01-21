@echo off

echo [WARNING] If you're in China Mainland, we recommend you to edit hosts file to speed up the download.

pause

echo Checking if scoop is installed...
if not exist "%USERPROFILE%\scoop" (
	echo Installing scoop...
	@powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://get.scoop.sh'))"
) else (
	echo Scoop is already installed.
)

echo Checking if g++ is installed...
g++ -v
if %errorlevel% neq 0 (
	echo Installing g++...
	scoop install gcc
) else (
	echo g++ is already installed.
)

echo Checking if C++ 20 is supported...
echo main() { return 0; } > test.cpp
g++ -std=c++20 test.cpp
if %errorlevel% neq 0 (
	echo C++ 20 is not supported in your environment. Installing latest version of g++...
	scoop uninstall gcc
	scoop install gcc
) else (
	del test.cpp
	del a.exe
	echo C++ 20 is supported.
)

g++ evaluator.cpp -o evaluator -std=c++20 -Ofast -Wall -Wextra

echo Checking if Node.js is installed...
node -v
if %errorlevel% neq 0 (
	echo Installing Node.js...
	scoop install nodejs
) else (
	echo Node.js is already installed.
)

mkdir log
mkdir problems
mkdir problems\data
mkdir record
mkdir submissions
mkdir temp
mkdir 若想使用Testlib版SPJ，请手动将testlib.h添加到C++头文件目录

npm install

echo Congratulations! You have successfully configured the environment.

echo run `node main.js` and open http://localhost:3000 in your browser to use the local judge system.

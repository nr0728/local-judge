# Local Judge for Windows

简体中文 | [English]()

## 安装

1. [下载本项目的 Source Code]()。
2. 解压后运行 `quick_configure.bat`。
   - 如果由于网络环境等因素无法运行，请确保你的 PATH 中有 `g++`, `node`；
   - 然后编译 `evaluator.cpp` 到 `evaluator.exe`；
   - 最后执行 `npm install`。
3. 执行 `node main.js`，Local Judge 将会在 <http://localhost:3000> 可用。
   - 3000 端口被占用了怎么办？修改 `main.js`，把 `PORT` 改为一个可用端口，然后使用你更改后的端口访问即可。

## 功能

1. 支持 C++ 传统题的评测，包括 SPJ。
2. 自动识别并对于测试数据进行人性化排序，不会出现 `1.in 10.in 2.in` 之类的问题。
3. 多种评测结果的支持，AC, WA, TLE, MLE, OLE, CE 等。
4. 评测记录的保存。每成功执行一个评测，将会自动保存。
5. 自定义题目 ID 和评测记录 ID，不仅仅是数字。
6. 还有更多功能等你探索！

## Bug Report / Feature Request

在此 GitHub 项目的 issue 中提出即可。PRs Welcome!

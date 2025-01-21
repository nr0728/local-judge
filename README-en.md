# Local Judge for Windows

[简体中文](https://github.com/nr0728/local-judge/blob/main/README.md) | English

## Installation

1. [Download the Source Code](https://github.com/nr0728/local-judge/archive/refs/heads/main.zip) of this project.
2. After extraction, execute `quick_configure.bat`.
   - If execution fails due to network issues or other factors, ensure `g++` and `node` are in your PATH;
   - Then compile `evaluator.cpp` to `evaluator.exe`;
   - Finally, run `npm install`.
3. Execute `node main.js`, and Local Judge will be accessible at <http://localhost:3000>.
   - What if port 3000 is occupied? Modify `PORT` in `main.js` to an available port number, then access using your modified port.

## Features

1. Supports evaluation of traditional C++ problems, including Special Judge (SPJ).
2. Intelligent test data recognition and user-friendly sorting, eliminating issues like `1.in 10.in 2.in`.
3. Comprehensive evaluation results, including AC, WA, TLE, MLE, OLE, CE, etc.
4. Automatic preservation of evaluation records upon successful completion.
5. Customizable problem IDs and evaluation record IDs, not limited to numerical values.
6. More features await your exploration!

## Bug Reports / Feature Requests

Feel free to raise issues on this GitHub project. Pull Requests are welcome!

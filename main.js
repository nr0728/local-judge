// main.js

const express = require('express');
const http = require('http');
const path = require('path');
const fs = require('fs');
const multer = require('multer');
const { spawn } = require('child_process');
const socketIo = require('socket.io');

// 初始化Express应用
const app = express();
const server = http.createServer(app);
const io = socketIo(server);

// 中间件解析JSON和URL编码的数据
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// 设置Multer存储引擎（内存存储，因为我们使用的是文本区域）
const storage = multer.memoryStorage();
const upload = multer({ storage: storage });

// 提供主页面
app.get('/', (req, res) => {
    res.send(`
    <!DOCTYPE html>
    <html lang="zh-CN">
    <head>
        <meta charset="UTF-8">
        <title>Local Judge</title>
        <!-- Semantic UI CSS 通过 CDN -->
        <link rel="stylesheet" href="https://gcore.jsdelivr.net/npm/semantic-ui/dist/semantic.min.css">
        <!-- jQuery 通过 CDN -->
        <script src="https://code.jquery.com/jquery-3.1.1.min.js"></script>
        <!-- Semantic UI JS 通过 CDN -->
        <script src="https://gcore.jsdelivr.net/npm/semantic-ui/dist/semantic.min.js"></script>
        <!-- Socket.io 通过 CDN -->
        <script src="/socket.io/socket.io.js"></script>
        <style>
            /* 自定义样式以使页面更紧凑 */
            .ui.grid > .column {
                padding: 1em;
            }
            .ui.form .field {
                margin-bottom: 0.5em;
            }
            .ui.container {
                margin-top: 20px;
            }
            textarea {
                min-height: 200px;
            }
            /* 评测输出圆角框和HTML渲染，去除固定高度和滚动条 */
            #evaluatorOutput {
                background-color: #f9f9f9;
                padding: 1em;
                font-family: Lato, 'Helvetica Neue', Arial, Helvetica, sans-serif; /* 使用 Semantic UI 默认字体 */
                white-space: pre-wrap;
                border: 1px solid #ddd;
                border-radius: 10px; /* 圆角边框 */
                /* 移除固定高度和滚动条 */
            }
            .ui.button {
                margin-top: 0.5em;
            }
        </style>
    </head>
    <body>
        <div class="ui container">
            <h1 class="ui header">Local Judge</h1>
            <p>欢迎使用 Local Judge！项目地址：<a href="https://github.com/nr0728/local-judge">GitHub</a>。</p>
        </div>
        <div class="ui container">
            <div class="ui two column stackable grid">
                <!-- 左侧：提交代码 -->
                <div class="column">
                    <h2 class="ui dividing header">提交代码</h2>
                    <form class="ui form" id="submitForm">
                        <div class="field">
                            <label>提交ID</label>
                            <input type="text" name="submissionId" placeholder="请输入提交ID" required>
                        </div>
                        <div class="field">
                            <label>代码</label>
                            <textarea name="code" placeholder="在此输入您的代码..." required></textarea>
                        </div>
                        <button class="ui button primary" type="submit">提交</button>
                    </form>

                    <!-- 覆盖确认模态框 -->
                    <div class="ui modal" id="confirmOverwrite">
                        <div class="header">确认覆盖</div>
                        <div class="content">
                            <p>该提交ID已存在。您确定要覆盖吗？</p>
                        </div>
                        <div class="actions">
                            <div class="ui deny button">取消</div>
                            <div class="ui positive right labeled icon button" id="confirmYes">
                                确定
                                <i class="checkmark icon"></i>
                            </div>
                        </div>
                    </div>

                    <!-- 自定义消息模态框 -->
                    <div class="ui modal" id="customMessageModal">
                        <div class="header" id="customModalHeader">提示</div>
                        <div class="content">
                            <p id="customModalMessage"></p>
                        </div>
                        <div class="actions">
                            <div class="ui button" id="customModalClose">关闭</div>
                        </div>
                    </div>
                </div>

                <!-- 右侧：评测代码 -->
                <div class="column">
                    <h2 class="ui dividing header">评测代码</h2>
                    <form class="ui form" id="evaluateForm">
                        <div class="three fields">
                            <div class="field">
                                <label>题目ID</label>
                                <input type="text" name="problemId" placeholder="请输入题目ID" required>
                            </div>
                            <div class="field">
                                <label>提交ID</label>
                                <input type="text" name="submissionIdForEvaluation" placeholder="请输入提交ID" required>
                            </div>
                            <div class="field">
                                <label>文件IO</label>
                                <select class="ui dropdown" name="fileIO">
                                    <option value="0" selected>否</option>
                                    <option value="1">是</option>
                                </select>
                            </div>
                        </div>
                        <div class="three fields">
                            <div class="field">
                                <label>输入文件</label>
                                <input type="text" name="inputFile" placeholder="例如: stdin 或输入文件名" value="stdin">
                            </div>
                            <div class="field">
                                <label>输出文件</label>
                                <input type="text" name="outputFile" placeholder="例如: stdout 或输出文件名" value="stdout">
                            </div>
                            <div class="field">
                                <label>时间限制</label>
                                <div class="fields">
                                    <!-- 使用 Semantic UI 的栅格系统调整宽度 -->
                                    <div class="ten wide field">
                                        <input type="number" name="timeLimit" placeholder="例如: 1000" value="1000" required>
                                    </div>
                                    <div class="seven wide field">
                                        <select class="ui dropdown" name="timeUnit">
                                            <option value="ms" selected>ms</option>
                                            <option value="s">s</option>
                                        </select>
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div class="three fields">
                            <div class="field">
                                <label>内存限制</label>
                                <div class="fields">
                                    <!-- 使用 Semantic UI 的栅格系统调整宽度 -->
                                    <div class="ten wide field">
                                        <input type="number" name="memoryLimit" placeholder="例如: 512" value="512" required>
                                    </div>
                                    <div class="seven wide field">
                                        <select class="ui dropdown" name="memoryUnit">
                                            <option value="KB">KB</option>
                                            <option value="MB" selected>MB</option>
                                            <option value="GB">GB</option>
                                        </select>
                                    </div>
                                </div>
                            </div>
                            <div class="field">
                                <label>特殊评测</label>
                                <select class="ui dropdown" name="specialJudge">
                                    <option value="0" selected>否</option>
                                    <option value="1">是</option>
                                </select>
                            </div>
                            <div class="field">
                                <label>检查器文件</label>
                                <input type="text" name="checkerFile" placeholder="例如: none 或检查器文件名" value="none">
                            </div>
                        </div>
                        <button class="ui button green" type="submit">开始评测</button>
                    </form>

                    <h2 class="ui dividing header">评测记录</h2>
                    <div class="ui segment" id="evaluationRecords"> </div>
                </div>
            </div>
        </div>

        <script>
            const socket = io();

            // 处理提交表单提交
            $('#submitForm').submit(function(event) {
                event.preventDefault(); // 防止默认表单提交

                const formData = {
                    submissionId: $('input[name="submissionId"]').val(),
                    code: $('textarea[name="code"]').val()
                };

                $.ajax({
                    url: '/submit',
                    type: 'POST',
                    data: JSON.stringify(formData),
                    contentType: 'application/json',
                    success: function(response) {
                        if (response.exists) {
                            // 如果submissionId已存在，显示确认覆盖模态框
                            $('#confirmOverwrite').modal('show');
                        } else {
                            // 使用自定义消息模态框显示成功消息
                            showMessage('成功', response.message);
                        }
                    },
                    error: function(err) {
                        // 使用自定义消息模态框显示错误消息
                        showMessage('错误', '提交代码时出错。');
                        console.error(err);
                    }
                });
            });

            // 处理覆盖确认
            $('#confirmYes').click(function() {
                const formData = {
                    submissionId: $('input[name="submissionId"]').val(),
                    code: $('textarea[name="code"]').val()
                };

                $.ajax({
                    url: '/overwrite',
                    type: 'POST',
                    data: JSON.stringify(formData),
                    contentType: 'application/json',
                    success: function(response) {
                        $('#confirmOverwrite').modal('hide');
                        // 使用自定义消息模态框显示成功消息
                        showMessage('成功', response.message);
                    },
                    error: function(err) {
                        // 使用自定义消息模态框显示错误消息
                        showMessage('错误', '覆盖代码时出错。');
                        console.error(err);
                    }
                });
            });

            // 初始化Semantic UI模态框
            $('#confirmOverwrite').modal({
                centered: false,
                blurring: true
            });

            // 处理评测表单提交
            $('#evaluateForm').submit(function(event) {
                event.preventDefault(); // 防止默认表单提交

                const formData = {
                    problemId: $('input[name="problemId"]').val(),
                    submissionIdForEvaluation: $('input[name="submissionIdForEvaluation"]').val(),
                    fileIO: $('select[name="fileIO"]').val(),
                    inputFile: $('input[name="inputFile"]').val(),
                    outputFile: $('input[name="outputFile"]').val(),
                    timeLimit: $('input[name="timeLimit"]').val(),
                    timeUnit: $('select[name="timeUnit"]').val(),
                    memoryLimit: $('input[name="memoryLimit"]').val(),
                    memoryUnit: $('select[name="memoryUnit"]').val(),
                    specialJudge: $('select[name="specialJudge"]').val(),
                    checkerFile: $('input[name="checkerFile"]').val()
                };

                $.ajax({
                    url: '/evaluate',
                    type: 'POST',
                    data: JSON.stringify(formData),
                    contentType: 'application/json',
                    success: function(response) {
                        showMessage('信息', response.message + \"<div class=\\\"ui segment\\\" id=\\\"evaluatorOutput\\\"><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br></div>\");
                        // 一秒后清空之前的评测输出
						setTimeout(function() {
							$('#evaluatorOutput').html('');
						}, 100);
                    },
                    error: function(err) {
                        showMessage('错误', '开始评测时出错。');
                        console.error(err);
                    }
                });
            });

            // 通过Socket.io监听评测输出
            socket.on('evaluatorOutput', function(data) {
                // 将输出内容作为HTML插入
                $('#evaluatorOutput').append(data);
            });

            // 初始化自定义消息模态框
            $('#customMessageModal').modal({
                centered: false,
                blurring: true
            });

            // 关闭自定义消息模态框
            $('#customModalClose').click(function() {
                $('#customMessageModal').modal('hide');
            });

            // 触发自定义消息模态框的函数
            function showMessage(title, message) {
                $('#customModalHeader').text(title);
                $('#customModalMessage').html(message);
                $('#customMessageModal').modal('show');
            }

			function loadEvaluationRecords() {
				$.ajax({
					url: '/evaluationRecords',
					type: 'GET',
					success: function(response) {
						$('#evaluationRecords').empty();
						response.forEach(function(record) {
							const recordId = record.split('.')[0];
							$('#evaluationRecords').append('<div class="ui segment" id="' + recordId + '"> 评测 ID: ' + recordId + '（点击查看详细信息）</div>');
							$('#' + recordId).click(function() {
								$.ajax({
									url: '/evaluationRecord/' + recordId,
									type: 'GET',
									success: function(response) {
										showMessage('评测记录', response);
									},
									error: function(err) {
										showMessage('错误', err.responseText);
									}
								});
							});
						});
					},
					error: function(err) {
						showMessage('错误', err.responseText);
					}
				});
			}
			loadEvaluationRecords();
			socket.on('evaluationFinished', function() {
				$.ajax({
					url: '/saveRecord',
					type: 'POST',
					data: JSON.stringify({ evaluatorOutput: $('#customModalMessage').html(), submissionId: $('input[name="submissionIdForEvaluation"]').val() }),
					contentType: 'application/json',
					success: function(response) {
						loadEvaluationRecords();
					}
				});
			});
        </script>
    </body>
    </html>
    `);
});

// 处理代码提交路由
app.post('/submit', upload.none(), (req, res) => {
    const { submissionId, code } = req.body;

    // 定义提交文件的路径
    const submissionPath = path.join(__dirname, 'submissions', `${submissionId}.cpp`);

    // 检查submissionId是否已存在
    if (fs.existsSync(submissionPath)) {
        // 如果存在，通知客户端确认覆盖
        return res.json({ exists: true, message: '提交ID已存在。是否覆盖？' });
    }

    // 将代码写入指定文件
    fs.writeFile(submissionPath, code, (err) => {
        if (err) {
            console.error('写入文件时出错:', err);
            return res.status(500).json({ message: '提交代码失败。' });
        }
        res.json({ exists: false, message: '代码提交成功。' });
    });
});

// 处理代码覆盖路由
app.post('/overwrite', upload.none(), (req, res) => {
    const { submissionId, code } = req.body;

    // 定义提交文件的路径
    const submissionPath = path.join(__dirname, 'submissions', `${submissionId}.cpp`);

    // 覆盖现有文件
    fs.writeFile(submissionPath, code, (err) => {
        if (err) {
            console.error('覆盖文件时出错:', err);
            return res.status(500).json({ message: '覆盖代码失败。' });
        }
        res.json({ message: '代码覆盖成功。' });
    });
});

// 处理代码评测路由
app.post('/evaluate', upload.none(), (req, res) => {
    const {
        problemId,
        submissionIdForEvaluation,
        fileIO,
        inputFile,
        outputFile,
        timeLimit,
        timeUnit,
        memoryLimit,
        memoryUnit,
        specialJudge,
        checkerFile
    } = req.body;

    // 转换时间和内存限制到统一单位 (ms 和 KB)
    let timeLimitMs = parseInt(timeLimit);
    if (timeUnit === 's') {
        timeLimitMs *= 1000; // 秒转毫秒
    }

    let memoryLimitKB;
    switch(memoryUnit) {
        case 'MB':
            memoryLimitKB = parseInt(memoryLimit) * 1024;
            break;
        case 'GB':
            memoryLimitKB = parseInt(memoryLimit) * 1024 * 1024;
            break;
        default:
            memoryLimitKB = parseInt(memoryLimit);
    }

    // 设置默认值
    const params = {
        '--problem': problemId || '1',
        '--submission-id': submissionIdForEvaluation || '1',
        '--file-io': fileIO || '0',
        '--input-file': inputFile || 'stdin',
        '--output-file': outputFile || 'stdout',
        '--time-limit': timeLimitMs || '1000', // 使用统一的ms单位
        '--memory-limit': memoryLimitKB || '524288', // 使用统一的KB单位
        '--special-judge': specialJudge || '0',
        '--checker-file': checkerFile || 'none'
    };

    // 构建参数数组
    const args = [];
    for (const key in params) {
        args.push(key, params[key]);
    }

    // 生成评测文件路径
    const evaluatorPath = path.join(__dirname, 'evaluator.exe');

    // 检查evaluator.exe是否存在
    if (!fs.existsSync(evaluatorPath)) {
        return res.status(500).json({ message: '评测程序未找到。' });
    }

    // 启动评测程序
    const evaluator = spawn(evaluatorPath, args, { shell: true });

    // 监听标准输出
    evaluator.stdout.on('data', (data) => {
        io.emit('evaluatorOutput', data.toString());
    });

    // 监听标准错误
    evaluator.stderr.on('data', (data) => {
        io.emit('evaluatorOutput', data.toString());
    });

    evaluator.on('close', (code) => {
		io.emit('evaluationFinished');
    });

    // 响应客户端
    res.json({ message: '' });
});

// 启动服务器，监听端口3000
const PORT = 3000;
server.listen(PORT, () => {
    console.log(`服务器正在运行，端口号：${PORT}`);
});

app.get('/evaluationRecords', (req, res) => {
	fs.readdir('record', (err, files) => {
		if (err) {
			res.status(500).send(err.message);
			return;
		}
		const records = files.map(file => file.split('.')[0]);	
			res.send(records);
		});
});
app.get('/evaluationRecord/:recordId', (req, res) => {
	const recordId = req.params.recordId;
	fs.readFile(`record\\${recordId}.txt`, 'utf8', (err, data) => {
		if (err) {
			res.status(500).send(err.message);
			return;
		}
		res.send(data);
	});
});

app.post('/saveRecord', upload.none(), (req, res) => {
	const { evaluatorOutput, submissionId } = req.body;
	fs.writeFile(`record\\${submissionId}.txt`, evaluatorOutput, (err) => {
		if (err) {
			res.status(500).send(err.message);
			return;
		}
		res.send('记录保存成功');
	});
});


### 代码实现说明
- 实现`submission.py`中的`succAndProbReward`函数和`peekingMDP`函数
- 实现`util.py`中的`PolicyIteration`类（参照`ValueIteration`类的实现方式）
- 无需修改其他文件的代码
### 测试代码
- `grader.py`中包含problem2.1和problem2.3的测试代码
- 通过运行命令`python grader.py`来测试代码实现的效果
- problem2.1的测试代码有两种模式，**basic**模式和**hidden**模式，**basic**模式包含一些输入输出样例，测试`succAndProbReward`实现是否正确；**hidden**模型通过在ValueIteration和PolicyIteration算法中调用`succAndProbReward`来检验代码实现是否正确。
- 实现过程中，可以通过只保留`grader.py`中的某个测试语句，而注释掉其余的测试语句，来检查某个函数是否实现正确。比如只保留`grader.addBasicPart('3a-basic', test3a, 5, description="Basic test for succAndProbReward() that covers several edge cases.")`而注释掉其余两个测试语句
### 作业提交说明
- 简答题，只需将答案写在pdf文件就可以
- 编程题，需要将实验结果写在pdf文档，并提交`submission.py`和`util.py`两个文件
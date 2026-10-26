# Project 3 Design Log
**Author:** Nickolai Tomczak **P2 code used:** Instructor Reference
**Tools used:** VS Code and WSL Ubuntu for editing, compiling, debugging, and testing the project. ChatGPT was used for syntax errors due to it not compiling originally and I could not figure the reason. 

## 1. Hash table
I implemented the ToolRegistry using a hash table with separate chaining. Each bucket contains a vector of unique pointers to Tool objects. I chose separate chaining because collisions are straightforward to handle, and it allows multiple tools to occupy the same bucket without requiring complicated probing logic. I considered open addressing, but separate chaining was simpler to implement while maintaining stable pointers to registered tools.
For hashing, I used the FNV-1a algorithm. It processes every character in a tool's name and combines them into a hash value. This is important because names with similar prefixes, such as tool_1 and tool_100, should not automatically collide. I rejected a simpler has hbased only on the first character and string length because it ignores most of the characters.
My registry starts with 16 buckets. I selected a maximum load-factor threshold of 0.75. Before an insertion would exceed this threshold, the table doubles its bucket count and redistributes the existing tools. This limits the average number of tools in each bucket while avoiding unnecessary resizing.
I tested the registry by inserting 10,000 tools named tool_0 through tool_9999. The results were:
Bucket count: 16,384
Load factor: 0.610352
FNV-1a maximum chain length: 6
Weak alternative hash maximum chain length: 9,000
The weak hash used the first character plus the string length. Because most tool names begin with the same character and have similar lengths, it produced extremely long chains. The FNV-1A result demonstrates much better distribution for this workload.
Insertion and lookup are expected O(1) operations when the hash distributes names evenly. Resizing takes O(n) because every registered tool must be redistributed, but the table doubles in size, so resizes become less frequent as the registry grows. Therefore, insertion has amortized O(1) expected complexity.

## 2. Pointer stability
The registry stores tools using std::unique_ptr. During resizing, I move the unique pointers into the new buckets rather than moving or copying the Tool objects themselves. Moving a unique pointer changes which container owns the object but does not change the object's memory address. Therefore, a Tool* previously returned by find() remains valid after later insertions and resizes, as long as the registry still owns that tool.

## 3. Calculator trace
My calculator uses two stacks: one for values and another for operators. The expression 2 * (3 + 4) - 5 is evaluated one token at a time, following operator precedence and parenthesis.
the stacks change as follows:
2: values = [2], Operators = []
*: Values = [2], Operators = [*]
(: Values = [2], Operators = [*, (]
3: Values = [2, 3], Operators = [*, (]
+:Values = [2, 3], Operators = [*, (, +]
4: Values = [2, 3, 4], Operators = [*, (, +]
): The calculator evaluates 3+4. Values = [2,7], Operators = [*]
-: Multiplication is evaluated first. values = [14], Operators = [-]
5: Values = [14, 5], Operators = [-]
End: Subtraction is evaluated. values = [9], Operators = []
The final answer is 9. This method correctly handles parentheses and operator precedence without using recursion.

## 4. Errors as values
I used return values and error strings instead of exceptions for expected failures. For example, Workspace::resolve() returns false with a reason when a path is invalid or attempts to escape the workspace. Similarly, calc_evaluate() returns false when it encounters invalid expressions or division by zero. The division-by-zero error specifically returns "division by zero".
These failures can result from ordinary user input, so they should not crash the application. Returning errors allow the caller to display useful feedback and continue running. Exceptions are more appropriate for unexpected failures that normal control flow cannot reasonably handle, such as certain resource allocation failures.
## 5. What I would design differently
I would improve the test suite by adding randomized tool names, more collision-heavy inputs, and larger stress tests. I would also add more malformed calculator expressions to verify error handling. Although all 17 current Project 3 tests pass, broader testing would provide greater confidence that the implementation behaves correctly beyond the examples I have already checked.
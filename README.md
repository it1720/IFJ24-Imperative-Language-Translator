# IFJ24-Imperative-Language-Translator
Program reads source code written in the IFJ24 language and translates it into the target language IFJcode24 (intermediate code)

If the translation completes without errors, the program returns an exit code of **0** (zero).
If an error occurs, the return value is as follows:

- **1** – Lexical analysis error (invalid structure of the current lexeme).
- **2** – Syntax analysis error (invalid program syntax, missing header, etc.).
- **3** – Semantic error: Undefined function or variable.
- **4** – Semantic error: Incorrect number/type of parameters in a function call; invalid type or improper disposal of a function's return value.
- **5** – Semantic error: Redefinition of a variable or function; assignment to a non-modifiable variable.
- **6** – Semantic error: Missing or excess expression in a function return statement.
- **7** – Semantic error: Type compatibility issue in arithmetic, string, or relational expressions; incompatible expression type (e.g., in an assignment).
- **8** – Semantic error: Type inference failure – the variable type is not specified and cannot be determined from the expression.
- **9** – Semantic error: Unused variable within its scope; a mutable variable that cannot be modified after initialization.
- **10** – Other semantic errors.
- **99** – Internal compiler error (unrelated to the input program, e.g., memory allocation failure).

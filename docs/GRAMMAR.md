## Aurum
### Program
- There is no `main` function
- If user wants to exit explicitly with an `exit code;`, they can, otherwise it exits with 0
- Semicolons are mandatory

### Typing
- We can declare without initialising.
- `mint name = value;` (immutable variable, can have type annotations for specificity)
- `bar name = value;` (mutable variable that **can change type**)
- `bar<type> name = value;` (mutable variable that **CANNOT change type**)
- If user declares `bar name;` then its type is $Null$
- User can use type annotations to specify type explicitly (`bar: type` or `mint: type`)

### Scopes
- Aurum is a block-scoped language. Meaning any variables declared inside blocks `{...}` like `if/for/while`, functions, or even just blocks on their own, are trapped inside and are destroyed at the end of the block.
- HOWEVER, if I want a variable declared differently for different conditions, I don't need to declare it above the `if-else` then define it inside, instead we can declare and define it inside the block itself but `hoist` it into its parent scope, so it's available after the `if-else` block is over.
   - The `hoist` keyword just prevents that variable from being destroyed at the end of *this* scope, *hoisting* it into the outer scope. At the end of which, it will be destroyed.
- To remove a variable from memory (so we can reuse the name for something else, for example), we use the keyword `del`
- `del` basically blinks the variable from existence. Useful in built-in functions for preventing variable-name overlap
- Also useful if you have an immutable variable that you want to be mutable, you can re-declare it safely by `del`eting its previous instance

### Comments
- Inline comments: `$$ comment`
- Block comment `$~ ... ~$`

### If Statements
- parentheses are optional
- curly braces are optional (if body is only 1 statement)
- colon is necessary
- ARCHITECTURALLY: elif are just sugared else ifs (because I like elif better)

```
if condition: {

} elif(condition): {

} else: {

}
```

### While loops
```
while True: {
   yada yada yada
}
```

### Functions
```
craft doThis(arg1, arg2) => returntypehint {

}

craft doThat(arg1: typehint1 | typehint2) { $$ return type ambiguous

}
```

## Node Tree
$$
\begin{align}
   \text{[NODE]} &\to representation\ /\ type
\\ \\
   \text{[program]} &\to [statement]^+
\\
   \text{[statement]} &\to
      \begin{cases}
         \text{[simple statement]}
            \begin{cases}
               exit\ [expression]; \\
               mint \text{ name} = [expression];\ \ \footnotesize{\text{// constant value}} \\
               bar \text{ name} = [expression];\ \ \footnotesize{\text{{// mutable value and type}}} \\
               bar\text{<}type\text{>}\ \text{name} = [expression];\ \ \footnotesize{\text{// mutable value, immutable type}} \\
               [identifier]++; \\
               [identifier]--;
            \end{cases} \\ \\
         \text{[block statement]} \to \{\ [statement]^*\ \} \\ \\
         \text{[selection statement]}
            \begin{cases}
               if-else,\ elif(cond) = else\{\ if(cond)\ \} \\
               switch
            \end{cases} \\ \\
         \text{[iteration statement]}
            \begin{cases}
               for \\ while
            \end{cases} \\
      \end{cases}
\\ \\
   [\text{expression}] &\to
      \begin{cases}
         [term]
            \begin{cases}
               (expresssion) \\
               integer\ literal \\
               [identifier] \\
               [unary\ expression]
                  \begin{cases}
                     -[expression] \\
                     !\ \ [expression] \\
                  \end{cases} \\
            \end{cases} \\
      \\
         [binary\ expression]
         \begin{cases}
            [arithmetic\ expressions] \\
            [boolean\ expressions]
         \end{cases}
      \end{cases}
\end{align}
$$

### Legend
[<node\>]<sup>*</sup> = 0 or more acceptable <br>
[<node\>]<sup>+</sup> = 1 or more acceptable <br>

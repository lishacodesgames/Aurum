## Language
### Program
- There is no `main` function
- If user wants to exit explicitly with an `exit code;`, they can, otherwise it exits with 0
- Semicolons are mandatory

### Typing
- We can declare without initialising.
- `mint name = value;` (immutable variable, can have type annotations for specificity)
- `bar name = value;` (mutable variable that **can change type**)
- `bar<type> name = value;` (mutable variable that **CANNOT change type**)
- If user declares `bar name;` then its type is $None$
- User can use type annotations to specify type explicitly (`bar: type` or `mint: type`)

### Scopes
- Like python and java (kinda?), unlike c++, aurum has no scopes.
- If a variable exists when it is called, it will work despite it not being created there itself
- To remove a variable, we use the keyword `del`
- `del` basically blinks the variable from existence. Useful in built-in functions for preventing variable-name overlap
- Also useful if you have an immutable variable that you want to be mutable, you can re-declare it safely by `del`eting its previous instance

### If Statements
```
if condition: {
   call this(); $$ void function
} elif otherCondition: {
   mint x = call that(); $$ not void function
} else: {
   call doThat(); $$ NOT void function, just ignored return value;
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
craft doThis(arg1, arg2): returntypehint {

}

craft doThat(arg1: typehint1 | typehint2) { $$ return type ambiguous

}
```

## Node Tree
$$
\begin{align}
   \text{[NODE]} &\to representation\ /\ type
\\ \\
   \text{[program]} &\to [statement]^*
\\
   \text{[statement]} &\to
      \begin{cases}
         exit\ [expression]; \\
         bar \text{ name} = [expression];\ \ \footnotesize{\text{// mutable value}} \\
         mint \text{ name} = [expression];\ \ \footnotesize{\text{// constant value}} \\
      \end{cases}
\\
   [\text{expression}] &\to
      \begin{cases}
         [term]
            \begin{cases}
               integer\ literal \\
               [identifier] \\
               [unary\ expression]
                  \begin{cases}
                     -[expression] \\
                     +[expression]
                  \end{cases} \\
            \end{cases} \\
      \\
         [binary\ expression]
            \begin{cases}
               [expression] &*&\ [expression] - prec. = 1 \\
               [expression] &/&\ [expression] - prec. = 1 \\
               [expression] &+&\ [expression] - prec. = 0 \\
               [expression] &-&\ [expression] - prec. = 0 \\
            \end{cases}
      \end{cases}
\end{align}
$$

### Legend
[<node\>]<sup>*</sup> = 0 or more of node acceptable <br>
[<node\>]<sup>+</sup> = 1 or more of node acceptable <br>

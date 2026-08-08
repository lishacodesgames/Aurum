## Try
### Program
- There is no $main$ function, but there is a clear end point where you do $exit\ value;$
- Semicolons are mandatory

### Typing
- We can declare without initialising.
- $mint\ name = value;$ (immutable variable)
- $bar\ name = value;$ (mutable variable that **can change type**)
- $bar\ name: type = value;$ (mutable variable that **CANNOT change type**)

### Todo
- Change variable declaration to include types somehow or change the keyword. *var* is too basic.

## Node Tree
$$
\begin{align}
   \text{[NODE]} &\to representation\ /\ type
\\ \\
   \text{[program]} &\to \text{[statement]}^*
\\
   \text{[statement]} &\to \text{(syntactically):}
      \begin{cases}
         exit\ \text{[expression]}; \\
         bar \text{ name} = \text{[expression]};\ \ \footnotesize{\text{// mutable value}} \\
         mint \text{ name} = \text{[expression]};\ \ \footnotesize{\text{ // constant value}} \\
      \end{cases}
\\
   [\text{exit}] &\to exit\ \text{[expression]};
\\
   [\text{expression}] &\to integer\ literal % for now
\end{align}
$$

### Legend
[<node\>]<sup>*</sup> = 0 or more of node acceptable <br>
[<node\>]<sup>+</sup> = 1 or more of node acceptable <br>

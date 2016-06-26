#msh
Practice implementation of a shell

### What's done:
* Command execution
* Pipes
* Readline/history support
* Builtin functions (cd, exit)
* Dynamic prompt (changes to reflect exit code of previous command and current directory)
* IO redirection
* Sane lexing + parsing (via flex and bison)
    * Supports quoted strings
* Background jobs

### What isn't:
* Setting environment variables
* Escape sequences
* Keeping track of background jobs
* stderr redirection
* Defining aliases/functions
* Anything else not mentioned in the above section

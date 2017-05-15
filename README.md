#Marcel the Shell (with shoes on)

Practice shell implementation  
Name is a reference to [this video](https://www.youtube.com/watch?v=VF9-sEbqDvU)

### What's done:
* Command execution
* Pipes
* Readline/history support
* Builtin functions (cd, exit)
* Dynamic prompt (changes to reflect exit code of previous command and current directory)
* IO redirection (stdin, stdout, stderr)
* Sane lexing + parsing (via flex and bison)
    * Supports quoted strings
* Proper job control
* Safe signal handling via queueing
* Setting environment variables per command

### What isn't:
* Set local variables
* Set environment variables for entire session (e.g. export)
* Escape sequences
* Defining aliases/functions
* Anything else not mentioned in the above section

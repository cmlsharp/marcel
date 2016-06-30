#Marcel the Shell (with shoes on)
Practice shell implementation

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
* Keeping track of background jobs

### What isn't:
* Setting environment variables
* Escape sequences
* stderr redirection
* Defining aliases/functions
* Anything else not mentioned in the above section
* Unit testing

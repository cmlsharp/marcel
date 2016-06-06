#msh
Practice implementation of a shell

### What's done:
* Command execution
* Pipes
* Readline/history support
* Builtin functions (cd, exit)
* Dynamic prompt (changes to reflect exit code of previous command and current directory)

### What isn't:
* IO redirection (e.g. echo "Hello, world!" > foo.txt)
* Sane lexical analysis. (This is really hacky right now, will eventually use flex or something similar)
* Background jobs
* Setting environment variables
* Anything else not mentioned in the above section

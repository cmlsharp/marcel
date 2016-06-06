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
* Any sort of robust lexical analysis (currently, any instance of the character '|' is interpreted as a pipe, the command echo "hello, world" outputs "hello world" with surrounding double quotes, etc.)
* Background jobs
* Setting environment variables
* Anything else not mentioned in the above section

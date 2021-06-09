# Minima
Minima is a small, portable, and fast programming language written in C.

## The Syntax
Minima's syntax is optimized for a fast byte-code translation at the expense of program verbosity. However, it's easy to parse and compile. You can read more about Minima's syntax [here.](https://github.com/TheRealMichaelWang/minima/wiki/syntax)

```
proc fib(n) {
	if n <= 1 {
		return n
	}
	return goproc fib(n - 1) + goproc fib(n - 2)
}
goproc fib(25)
```

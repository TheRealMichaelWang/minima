# Minima
Minima is a small, portable, and dynamic object-oriented programming language written in C.

## The Syntax
Minima's syntax is optimized for a fast byte-code translation at the expense of program verbosity. However, it's easy to parse and compile. You can read Minima's documentation [here.](https://github.com/TheRealMichaelWang/minima/wiki)

Countdown
```
set i to 10
while dec i {
	extern printl(i)
}
```

## Some Examples

Fizz Buzz
```
set i to 0
while inc i <= 99 {
	if i % 15 == 0 {
		extern print("fizzbuzz")
	}
	elif i % 5 == 0{
		extern print("fizz")
	}
	elif i % 3 == 0{
		extern print("buzz")
	}
	else {
		extern print(i)
	}
	extern printl
}
```

Fibonacci
```
proc fib(n) {
	if n <= 1 {
		return n
	}
	return goproc fib(n - 1) + goproc fib(n - 2)
}
extern print(goproc fib(25))
```

Factorial
```
proc fact(n) {
	if n {
		return n * goproc fact(n - 1)
	}
	return 1
}
extern print(goproc fact(50))
```

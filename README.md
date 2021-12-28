<p align="center">
  <img src="https://repository-images.githubusercontent.com/371836745/8d964bac-a4d2-4e19-b8f3-2e2522bec8a1" width="350" title="hover text">
</p>

# Minima
Minima is an object-oriented, imperative scripting language focused on being practical and pragmatic. Minima aims to be simple, easy to write, yet powerful enough to create readable, performant and concise code for any problem. You can read [Minima's documentation here](https://github.com/TheRealMichaelWang/minima/wiki), and view the [installation guide here](https://github.com/TheRealMichaelWang/minima/wiki/Installation).

Among many things, it's...
* **Small** - The compiler and virutal-machine clock slightly under 5000 lines of code. 
* **Portable** - Minima compiles byte-code and runs it on a virtual-machine. It's the same way Java does it.
* **Powerful** - Minima is designed for quick-and-messy object-oriented programming on the fly. 
* **Embeddable** - It's very easy to imbed it into your C/C++ project.
* **Easy To Learn** - You could probably read the documentation, and learn the language, in under half-an-hour.

## Examples

### Count Down 
```
set i to 10
while dec i {
	extern printl(i)
}
```

### Fizz Buzz
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

### Fibonacci
```
proc fib(n) {
	if n <= 1 {
		return n
	}
	return goproc fib(n - 1) + goproc fib(n - 2)
}
extern print(goproc fib(25))
```

### Factorial
```
proc fact(n) {
	if n {
		return n * goproc fact(n - 1)
	}
	return 1
}
extern print(goproc fact(50))
```

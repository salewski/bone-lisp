![Bone Lisp](logo.png)

# The Bone Lisp programming language

Note: This software seems to work pretty well, but it's neither full-featured nor battle-tested, so it should be considered beta-quality.
Furthermore, it will probably stay there forever as I have moved on to work on other projects.
If you are interested in researching the approach of combining explicit regions and immutability, don't hesitate to contact me if you want to know what I would do differently today compared to what I did here.

    (defsub (len xs)
      "Calculate the length of the list `xs`."
      (with loop (lambda (remain n)
                   (if (nil? remain)
                       n
                     (loop (cdr remain) (++ n))))
        (loop xs 0)))

## What?

Bone is an interpreter for a lexically scoped Lisp-1.
It is based on immutable values and does tail-call elimination.
The special feature that distinguishes it from other Lisps is the *semi-automatic memory management*: 
It uses explicit regions instead of garbage collection.

It is inspired by Pico Lisp, R5RS Scheme, Forth, Common Lisp and Ruby.

It is written for 64 bit systems and runs on GNU/Linux,
  though it should also work on other Unices (with minimal porting).

*Note: If you were looking for the great [Bones Scheme Compiler](http://www.call-with-current-continuation.org/bones/), that's a different project.*

## Why?

Garbage collection is not a solved problem.
It becomes extremely complex if you want to support multi-threading, avoid pause-times and handle large heaps well.
But doing manual memory management is time-consuming for the programmer - and usually also error-prone.
Explicit regions are both very simple and very fast, but how far can one get with them?
I want to find out, so I am developing this interpreter.

A language like Bone Lisp could maybe become useful for soft real-time systems (e.g. as a scripting language for games), some kinds of multi-threaded servers and maybe even embedded systems with enough memory.

## Status

[![Build Status](https://travis-ci.org/wolfgangj/bone-lisp.svg?branch=master)](https://travis-ci.org/wolfgangj/bone-lisp)

### What it does

* Lexical scoping & closures
* Optional dynamic scoping
* Tail call elimination
* Explicit regions memory management
* Lists, symbols, strings, fixnums, floats
* Classic Lisp Macros
* Reader macros
* I/O streams with UTF-8 support
* TAS conforming testing library
* POSIX bindings (but not many yet...)

### What it does not

* Multithreading (did not get around to implementing it)
* Networking (likewise)

* Arrays
  (don't fit too well into Bone)
* Hash tables
  (we have an implementation anyway, but same as for arrays applies)
* Exceptions
* Module system
  (our current hyper-static environment might be good enough)
* Bignums
* Records / structures
* Keywords
  (they are nice, but make things more complex)
* Cross-cutting concerns
  (probably very useful for the memory region stuff)

* Garbage collection
  (obviously, since the whole point of Bone Lisp is to avoid it)
* Continuations
  (I don't think they make sense with explicit regions)
* Being compatible to other Lisp dialects
* Object oriented programming
  (creating a good object system is hard and thus takes a lot of time I'd rather spend on other features)

## Getting started

To make embedding as easy as possible, the whole interpreter is in a single C file.
Optional modules have their own C files (currently only `boneposix.c` for the POSIX bindings).
The `main` function is in `main.c`;
it just initializes everything and calls the REPL.
You can compile it all with `make`.

## Quick Intro

Bone Lisp doesn't try to be an overly innovative Lisp (like e.g. Clojure), nor does it try hard to be compatible with tradition.
I hope you'll like the few things Bone does different than traditional Lisps.

One important piece of terminology is changed:
Keeping with the times, we reserve the term "function" for pure functions without side-effects.
Since Bone Lisp allows some side-effects (like I/O), we mostly speak about using subroutines in our code.
Usually, we abbreviate "subroutine" as "sub", like it is done in modern BASIC dialects.

To the usual syntactic sugar (like `'x` for quoting) we only add a shortcut for subs with a single expression in the body:

    | a b c (foo)   ; => (lambda (a b c) (foo))

We use this only for one-liners, though.

Rest arguments work like they do in Scheme:

    (lambda args foo)
    (lambda (a b c . args) foo)

Booleans and the empty list work almost like they do in Scheme:

* The empty list is written as `()` and is self-evaluating (whereas in Scheme it can't be evaluated).
* While we still call the empty list "nil", it is not the symbol `nil` (which isn't special in any way).
* You cannot take the `car` and `cdr` of the empty list.
* Only the value `#f` is false.
* The canonical value for true is `#t`.

The names of predicates end with a question mark (e.g. `nil?`).
Subs which may return a useful value or `#f` (false) also follow this convention (eg. `assoc?`).
This helps to prevent forgetting about the possbility of returning `#f`.

Most names in the library are taken from Scheme and Common Lisp.
Often, we provide several names for the same thing (like Ruby does).
For example, `len`, `length` and `size` are the same.
See `core.bn` for docstrings describing the builtins.
(We also have `gendoc.bn`, which extracts the docstrings from a source file and generates a markdown file from them.
But this needs improvement.)

A subroutine can be defined with `defsub`; note that the docstring is required!

    (defsub (sum xs)
      "Add all numbers in `xs`."
      (apply + xs))
    
    (sum '(1 2 3))  ; => 6

For local helper subs you can use `mysub`, which does not require a docstring and introduces a binding that may be overwritten later.
Note that the environment is hyperstatic (as in Forth):
If you redefine something, the subs previously defined will continue to use the old definition.

Quasiquoting works as usual, so you can define macros (with `defmac` or `mymac`):

    (defmac (when expr . body)
      "Evaluate all of `body` if `expr` is true."
      `(if ,expr (do ,@body)))

The primitive form that introduces a (single) new binding - which may be recusive as in Schemes `letrec` - is `with`:

    (with loop | xs (if (nil? xs)
                        0
                      (++ (loop (cdr xs))))
      (loop '(a b c d)))
    ;; => 4

`let` is simply defined as a macro that expands to nested `with`s.
Therefore it works like a combination of traditional `let*` and `letrec`.

Dynamic scope can be used by defining a variable first with `defvar`.
Then you can set it for the dynamic extent of some expressions with `with-var`:

    (defvar *depth* 0)
    (with-var *depth* (++ *depth*)
      (list *depth*))
    ;; => (1)
    
    *depth*
    ;; => 0

The use of regions is available via:

    (in-reg expr1 expr2 ...)

The given `expr`s will be evaluated with objects allocated in a new region.
The return value (of the last `expr`) will be copied to the previous region.
Finally, the new region will be freed.

A source file should declare which version of Bone Lisp it was written for:

    (version 0 4)  ; for v0.4.x

There is no real module system, but you can load files with:

    (use foo)

This will load `foo.bn`.
Each file will be loaded only once.
Recursive loading will be detected and reported as an error.

There's much more, but currently you'll have to look into the source code.
If you don't understand something, feel free to ask.

## License

This is Free Software distributed under the terms of the ISC license.
(A very simple non-copyleft license.)
See the file LICENSE for details.

## Who?

It is being developed by
Wolfgang Jaehrling (wolfgang at conseptizer dot org)

## Links

Bone Lisp is influenced by:
* [PicoLisp](http://picolisp.com/) is a pragmatic but simple Lisp
* [R5RS Scheme](http://www.schemers.org/Documents/Standards/R5RS/) is a beautiful Lisp dialect
* [Forth](https://en.wikipedia.org/wiki/Forth_%28programming_language%29) is a deep lesson in simplicity
* [Common Lisp](https://common-lisp.net/) is a full-featured traditional Lisp
* [Ruby](https://www.ruby-lang.org/) is a scripting language with great usability

Somewhat related Free Software projects:
* [Pre-Scheme](https://en.wikipedia.org/wiki/PreScheme) is a GC-free (LIFO) subset of Scheme
* [Carp](https://github.com/eriksvedang/Carp) is "a statically typed lisp, without a GC"
* [newLISP](http://www.newlisp.org/) uses "One Reference Only" memory management
* [MLKit](http://www.elsman.com/mlkit/) uses region inference (and a GC)
* [Linear Lisp](http://home.pipeline.com/~hbaker1/LinearLisp.html) produces no garbage
* [Dale](https://github.com/tomhrr/dale) is basically C in S-Exprs (but with macros)
* [ThinLisp](http://www.thinlisp.org/) is a subset of Common Lisp that can be used without GC

Roadmap: The Path to a "Reasonably Developed" ManiT

1. Core Type System 🧱

Our type system is currently very simple and often assumes i32. We need to make it formal and expand it.

    Achieved: Integers (i32), Booleans (i1).

    Next Steps:

        More Primitive Types: char, floating-point numbers (f32, f64), and other integer sizes (i8, i16, i64, u32, etc.).

        Explicit Type Annotations: The ability to write let x: i64 = 5; to explicitly state a variable's type. This is a core tenet of Rust's safety and our philosophy of Explicitness.

        Type Checking: A "semantic analysis" pass in the compiler that walks the AST to ensure type correctness (e.g., you can't add a boolean to an integer).

2. Aggregate Data Types 📦

Programs need to group data together. This is a high-priority area.

    Achieved: Arrays ([N x T]).

    Next Steps:

        Strings: A proper, easy-to-use string type is one of Python's greatest strengths. We must implement this. It will likely be built on top of arrays of characters.

        Structs: This is the cornerstone of creating complex data types, just like in Rust. It allows us to define our own structures, like struct Point { x: i32, y: i32 }. This is probably our most important next feature.

        Enums: Another powerful feature from Rust for creating types that can be one of several variants (e.g., enum Color { Red, Green, Blue }).

3. Pointers and Memory Management 🧠

As a systems language intended to build an OS, this is non-negotiable.

    Achieved: Basic stack allocation (alloca).

    Next Steps:

        Pointers/References: The ability to get the memory address of a variable (&x), and dereference a pointer (*p). This is fundamental for low-level programming.

        Heap Allocation: A mechanism to request memory at runtime (e.g., malloc/free equivalents) for data whose size isn't known at compile time.

4. Language Usability and Modularity 🚀

These are features that make the language practical for larger projects.

    Achieved: Basic functions and control flow.

    Next Steps:

        Module System: The ability to split code into multiple files and import them, as seen in Rust and Python.

        Standard Library: A set of built-in, useful functions. A print() function would be a great first addition.

        Advanced Error Handling: A mechanism better than crashing, perhaps inspired by Rust's Result<T, E> and Option<T> types.


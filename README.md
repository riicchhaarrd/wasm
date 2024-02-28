# Parsing WASM example in C

```
$ g++ parse.c -o parse && ./parse
magic = , a, s, m
version = 1
8/293
Section type (1) (9 bytes)
reading section type
tag=96
() -> []
tag=96
(i32 %0) -> [i32]
19/293
Section function (3) (3 bytes)
0 -> 0
1 -> 1
24/293
Section memory (5) (3 bytes)
memory 0: min:2, max:0, present:0
29/293
Section global (6) (24 bytes)
0: mutable 0, valtype i32
i32 const 1024
1: mutable 0, valtype i32
i32 const 1028
2: mutable 0, valtype i32
i32 const 1024
3: mutable 0, valtype i32
i32 const 66576
4: mutable 0, valtype i32
i32 const 0
5: mutable 0, valtype i32
i32 const 1
67/293
Section export (7) (7e bytes)
        export 'memory', type = 2, idx = 0
        export '__wasm_call_ctors', type = 0, idx = 0
        export 'test', type = 0, idx = 1
        export '__dso_handle', type = 3, idx = 0
        export '__data_end', type = 3, idx = 1
        export '__global_base', type = 3, idx = 2
        export '__heap_base', type = 3, idx = 3
        export '__memory_base', type = 3, idx = 4
        export '__table_base', type = 3, idx = 5
195/293
Section code (10) (24 bytes)
read_section_code n = 2
233/293
Section data (11) (9 bytes)
244/293
Section custom (0) (2f bytes)
```

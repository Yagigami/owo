[ ] make a direct (stack-allocated?) light hash map for quick lookup in small tables
[ ] use a stack data structure to keep local variables (may even be good enough not to have hashmaps for locals)
	~~ - linear search from the end (that allows shadowing!) ~~
	- linear search from the start (fuck shadowing)
	- bloom filter (per scope?)
[ ] get a good hash for 16 byte idents
[ ] when types are added, intern and hash them to reduce pointer chasing when type-checking
[ ] discard small_buf
[ ] make temporaries temporary, not keep them in the AST (bc_unit's local_syms)
[ ] make functions `static` a lot more often
[ ] redo the headers they're terrible
[x] bytecode layout for 32-bit instructions (8 opc, 3x8 ops, opcode extensions if necessary in the future by reserving a few opcodes)
	- operands are 1 byte indices in a local symbol table with flat types
	- immediates/global references stored as a bigger entity (32-bit?)
[x] make a cool layout table for bytecode (like ```c
		struct layout {
			uint8_t operands: MAX_OPS;
			uint8_t operand_switch: 1 << MAX_OPS; // [x] = (kind != REF)
			...
		};
		layouts[BC_SETI] = { 2, REF | (IMM << 1) };
	```) for pretty printing and stuffs
[ ] dump more AST


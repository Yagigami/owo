#include "common.h"
#include "x86-64.h"
#include "elf-rename-me.h"
#include "alloc.h"

#include <elf.h>
#include <string.h>


static void pad(allocator al, vector *v, len_t n)
{
	vec_reserve(al, v, n, 1);
	v->len += n;
}

stream elf_serialize_x64(allocator al, const gen_x64 *gen, const char *file)
{
	vector v = { 0 };
	Elf64_Ehdr ehdr;
	ehdr.e_ident[EI_MAG0       ] = ELFMAG0;
	ehdr.e_ident[EI_MAG1       ] = ELFMAG1;
	ehdr.e_ident[EI_MAG2       ] = ELFMAG2;
	ehdr.e_ident[EI_MAG3       ] = ELFMAG3;
	ehdr.e_ident[EI_CLASS      ] = 2; // 64 bit
	ehdr.e_ident[EI_DATA       ] = 1; // little endian
	ehdr.e_ident[EI_VERSION    ] = 1; // ELF version 1
	ehdr.e_ident[EI_OSABI      ] = 0x00; // System V ABI
	ehdr.e_ident[EI_ABIVERSION ] = 0x00;
	ehdr.e_type = 0x1; // relocatable object file
	ehdr.e_machine = 0x3e; // AMD64
	ehdr.e_version = 1; // ELF version 1
	ehdr.e_entry = 0x0; // no entry point
	ehdr.e_phoff = 0; // no program header
	// ehdr.e_shoff set later
	ehdr.e_flags = 0;
	ehdr.e_ehsize = sizeof ehdr;
	ehdr.e_phentsize = sizeof (Elf64_Phdr);
	ehdr.e_phnum = 0;
	ehdr.e_shentsize = sizeof (Elf64_Shdr);
	// ehdr.e_shnum set later
	// ehdr.e_shstrndx set later

	// will do 6 sections for now:
	//   UNDEF, .text, .symtab, .strtab, .shstrtab, .rela.text
	enum {
		SEC_UNDEF = 0,
		SEC_TEXT,
		SEC_SYMTAB,
		SEC_STRTAB,
		SEC_SHSTRTAB,
		SEC_RELA_TEXT,
		SEC_NUM,
	};
	Elf64_Shdr shdr[SEC_NUM];
	// section UNDEF
	memset(shdr, 0, sizeof shdr[SEC_UNDEF]);
	// section .text
	shdr[SEC_TEXT].sh_name = 1;
	shdr[SEC_TEXT].sh_type = SHT_PROGBITS;
	shdr[SEC_TEXT].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	shdr[SEC_TEXT].sh_addr = 0x0; // relocatable file, not executable
	// shdr[SEC_TEXT].sh_offset
	shdr[SEC_TEXT].sh_size = sm_len(gen->insns);
	shdr[SEC_TEXT].sh_link = 0x0;
	shdr[SEC_TEXT].sh_info = 0x0;
	shdr[SEC_TEXT].sh_addralign = 0x10;
	shdr[SEC_TEXT].sh_entsize = 0;
	// section .symtab
	Elf64_Sym *syms = gen_alloc(al, (3 + sm_len(gen->syms)) * sizeof *syms);
	memset(syms, 0, sizeof syms[0]);
	syms[1].st_name = 1;
	syms[1].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FILE);
	syms[1].st_other = STV_DEFAULT;
	syms[1].st_shndx = 0xfff1; // SHN_ABS
	syms[1].st_value = 0x0;
	syms[1].st_size = 0x0;

	syms[2].st_name = 0;
	syms[2].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
	syms[2].st_other = STV_DEFAULT;
	syms[2].st_shndx = SEC_TEXT;
	syms[2].st_value = 0x0;
	syms[2].st_size = 0x0;
	shdr[SEC_SYMTAB].sh_name = 7;
	shdr[SEC_SYMTAB].sh_type = SHT_SYMTAB;
	shdr[SEC_SYMTAB].sh_flags = 0x0;
	shdr[SEC_SYMTAB].sh_addr = 0;
	// shdr[SEC_SYMTAB].sh_offset
	shdr[SEC_SYMTAB].sh_size = (
		+ 1 /* undefined symbol */
		+ 1 /* file name symbol */
		+ 1 /* section symbol */
		+ sm_len(gen->syms)) * sizeof (Elf64_Sym);
	shdr[SEC_SYMTAB].sh_link = SEC_STRTAB; // index of associated string table
	shdr[SEC_SYMTAB].sh_info = 3; // 1 greater than last local symbol (section thing) 's index
	shdr[SEC_SYMTAB].sh_addralign = 0x8;
	shdr[SEC_SYMTAB].sh_entsize = sizeof (Elf64_Sym);
	// section .strtab
	vector strtab = { 0 };
	char *cur = vec_reserve(al, &strtab, 1 /* empty string */, 1);
	cur[strtab.len++] = '\0';
	cur = vec_reserve(al, &strtab, strlen(file) + 1, 1);
	memcpy(cur + strtab.len, file, strlen(file) + 1);
	strtab.len += strlen(file) + 1;
	len_t syms_idx = 3;
	len_t code_offset = 0;
	for (struct cg_sym *start = sm_mem(gen->syms), *end = start + sm_len(gen->syms)
			, *it = start; it != end; it++) {
		const struct cg_sym sym = *it;
	// sm_iter(struct cg_sym, gen->syms, sym, {
		// len_t l;
		// const char *s = repr_ident(sym.name, &l);
		// printf("\"%.*s\"\n", (int) l, s);
		cur = vec_reserve(al, &strtab, fb_len(sym.name) + 1, 1);
		memcpy((char *) strtab.mem + strtab.len, fb_mem(sym.name), fb_len(sym.name));
		syms[syms_idx].st_name = strtab.len;
		strtab.len += fb_len(sym.name);
		cur[strtab.len++] = '\0';
		syms[syms_idx].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
		syms[syms_idx].st_other = STV_DEFAULT;
		syms[syms_idx].st_shndx = SEC_TEXT;
		syms[syms_idx].st_value = code_offset;
		syms[syms_idx].st_size = sym.size;
		code_offset += sym.size;
		syms_idx++;
	}
	// });

	// printf("\n");
	// for (const char *c = cur, *end = c + strtab.len; c != end; c++) {
		// printf("%02x ", (unsigned) *c);
	// }
	// printf("\n");

	shdr[SEC_STRTAB].sh_name = 15;
	shdr[SEC_STRTAB].sh_type = SHT_STRTAB;
	shdr[SEC_STRTAB].sh_flags = SHF_STRINGS;
	shdr[SEC_STRTAB].sh_addr = 0;
	// shdr[SEC_STRTAB].sh_offset
	shdr[SEC_STRTAB].sh_size = strtab.len;
	shdr[SEC_STRTAB].sh_link = 0x0;
	shdr[SEC_STRTAB].sh_info = 0x0;
	shdr[SEC_STRTAB].sh_addralign = 1;
	shdr[SEC_STRTAB].sh_entsize = 0;
	// section .shstrtab
	char shstrtab[] = "\0.text\0.symtab\0.strtab\0.shstrtab\0.rela.text";
	shdr[SEC_SHSTRTAB].sh_name = 23;
	shdr[SEC_SHSTRTAB].sh_type = SHT_STRTAB;
	shdr[SEC_SHSTRTAB].sh_flags = SHF_STRINGS;
	shdr[SEC_SHSTRTAB].sh_addr = 0; // not loaded
	// shdr[SEC_SHSTRTAB].sh_offset 
	shdr[SEC_SHSTRTAB].sh_size = sizeof shstrtab;
	shdr[SEC_SHSTRTAB].sh_link = 0x0; // idk
	shdr[SEC_SHSTRTAB].sh_info = 0x0;
	shdr[SEC_SHSTRTAB].sh_addralign = 1;
	shdr[SEC_SHSTRTAB].sh_entsize = 0;
	// section .rela.text
	memset(shdr + SEC_RELA_TEXT, 0, sizeof *shdr);
	shdr[SEC_RELA_TEXT].sh_addralign = 1;
	// TODO

	len_t sz = 0;
	vec_reserve(al, &v, sizeof ehdr, 1);
	v.len += sizeof ehdr;
	// vec_extend(al, &v, &ehdr, sizeof ehdr, 1);
	sz = ALIGN_UP_P2(sz + sizeof ehdr, shdr[SEC_TEXT].sh_addralign);
	shdr[SEC_TEXT].sh_offset = sz;
	vec_extend(al, &v, sm_mem(gen->insns), shdr[SEC_TEXT].sh_size, 1);

	sz = ALIGN_UP_P2(sz + shdr[SEC_TEXT].sh_size, shdr[SEC_TEXT + 1].sh_addralign);
	pad(al, &v, sz - v.len);
	shdr[SEC_SYMTAB].sh_offset = sz;
	vec_extend(al, &v, syms, shdr[SEC_SYMTAB].sh_size, 1);

	sz = ALIGN_UP_P2(sz + shdr[SEC_SYMTAB].sh_size, shdr[SEC_SYMTAB + 1].sh_addralign);
	pad(al, &v, sz - v.len);
	shdr[SEC_STRTAB].sh_offset = sz;
	vec_extend(al, &v, strtab.mem, shdr[SEC_STRTAB].sh_size, 1);

	sz = ALIGN_UP_P2(sz + shdr[SEC_STRTAB].sh_size, shdr[SEC_STRTAB + 1].sh_addralign);
	pad(al, &v, sz - v.len);
	shdr[SEC_SHSTRTAB].sh_offset = sz;
	vec_extend(al, &v, shstrtab, shdr[SEC_SHSTRTAB].sh_size, 1);

	sz = ALIGN_UP_P2(sz + shdr[SEC_SHSTRTAB].sh_size, shdr[SEC_SHSTRTAB + 1].sh_addralign);
	pad(al, &v, sz - v.len);
	// 
	shdr[SEC_RELA_TEXT].sh_offset = sz;

	sz = ALIGN_UP_P2(sz + shdr[SEC_RELA_TEXT].sh_size, 0x10);
	pad(al, &v, sz - v.len);
	vec_extend(al, &v, &shdr, sizeof shdr, 1);
	ehdr.e_shoff = sz;

	sz = ALIGN_UP_P2(sz + sizeof shdr, 0x10);
	pad(al, &v, sz - v.len);
	ehdr.e_shnum = SEC_NUM;
	ehdr.e_shstrndx = SEC_SHSTRTAB;
	memcpy(v.mem, &ehdr, sizeof ehdr);

	gen_free(al, syms, shdr[SEC_SYMTAB].sh_size);
	gen_free(al, strtab.mem, strtab.cap);
	v.mem = gen_realloc(al, sz, v.mem, v.cap);

	stream s;
	s.buf = v.mem;
	s.len = v.len;
	return s;
}


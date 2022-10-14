#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

u8* read_file(const char* path, size_t* sz)
{
	u8* buf = 0;
	FILE* fp = fopen(path, "rb");
	if (!fp)
		return 0;
	fseek(fp, 0, SEEK_END);
	*sz = ftell(fp);
	rewind(fp);
	buf = (u8*)malloc(*sz);
	fread(buf, 1, *sz, fp);
	fclose(fp);
	return buf;
}

typedef struct
{
	size_t cursor, sz;
	u8* buf;
} stream_t;

void stream_init(stream_t* s, u8* buf, size_t sz)
{
	s->cursor = 0;
	s->buf = buf;
	s->sz = sz;
}

u8 stream_get(stream_t *s)
{
	if (s->cursor >= s->sz)
		return 0;
	return s->buf[s->cursor++];
}

int stream_read(stream_t* s, void* vbuf, size_t n)
{
	u8 *buf = (u8*)vbuf;
	for (size_t i = 0; i < n; ++i)
	{
		buf[i] = stream_get(s);
	}
	return 0;
}

u32 stream_decode_leb_128(stream_t* s)
{
	u32 result = 0;
	u32 shift = 0;
	while (1)
	{
		u8 byte = stream_get(s);
		result |= (byte & 0x7f) << shift;
		if(!(byte & 0x80))
			break;
		shift += 7;
	}
	return result;
}

#define STREAM_READ(x, t) stream_read(&x, (u8*)&t, sizeof(t))

typedef enum
{	
    k_ESectionIdCustom = 0,
    k_ESectionIdType = 1,
    k_ESectionIdImport = 2,
    k_ESectionIdFunction = 3,
    k_ESectionIdTable = 4,
    k_ESectionIdMemory = 5,
    k_ESectionIdGlobal = 6,
    k_ESectionIdExport = 7,
    k_ESectionIdStart = 8,
    k_ESectionIdElement = 9,
    k_ESectionIdCode = 10,
    k_ESectionIdData = 11,
    k_ESectionIdDataCount = 12
} k_ESectionId;

static const char* section_id_strings[] = {"custom", "type",  "import",	 "function", "table", "memory",		"global",
										   "export", "start", "element", "code",	 "data",  "data count", NULL};

typedef enum
{
	k_EValueTypeNumberi32 = 0x7f,
	k_EValueTypeNumberi64 = 0x7e,
	k_EValueTypeNumberf32 = 0x7d,
	k_EValueTypeNumberf64 = 0x7c,
	k_EValueTypeVector = 0x7b,
	k_EValueTypeReferenceFunc = 0x70,
	k_EValueTypeReferenceExtern = 0x6f
} k_EValueType;

static const char* value_type_to_string(u8 type)
{
	switch (type)
	{
		case k_EValueTypeNumberi32:
			return "i32";
		case k_EValueTypeNumberi64:
			return "i64";
		case k_EValueTypeNumberf32:
			return "f32";
		case k_EValueTypeNumberf64:
			return "f64";
		case k_EValueTypeVector:
			return "vector";
		case k_EValueTypeReferenceFunc:
			return "funcref";
		case k_EValueTypeReferenceExtern:
			return "externref";
	}
	return NULL;
}

void read_section_type(stream_t* s)
{
	printf("reading section type\n");
	size_t numtypes = stream_get(s);
	for (size_t i = 0; i < numtypes; ++i)
	{
		u8 tag = stream_get(s);
		assert(tag == 0x60);
		u32 numparms = stream_decode_leb_128(s);
		/* printf("numparms = %d\n", numparms); */
		printf("(");
		for (size_t j = 0; j < numparms; ++j)
		{
			u8 value_type = stream_get(s);
			const char *vts = value_type_to_string(value_type);
			/* printf("parm %d = %s\n", j, vts); */
			printf("%s %%%d%s", vts, j, j == numparms - 1 ? "" : ", ");
		}
		printf(")");
		printf(" -> ");
		u32 numresults = stream_decode_leb_128(s);
		/* printf("numresults = %d\n", numresults); */
		printf("[");
		for (size_t j = 0; j < numresults; ++j)
		{
			u8 value_type = stream_get(s);
			const char *vts = value_type_to_string(value_type);
			/* printf("result %d = %s\n", j, vts); */
			printf("%s%s", vts, j == numresults - 1 ? "" : ", ");
		}
		printf("]");
		printf("\n");
	}
}

int read_string(stream_t *s, char *out, size_t outlen)
{
    u32 numbytes = stream_decode_leb_128(s);
    size_t n = 0;
    while (n < numbytes)
    {
        if (n >= outlen)
        {
			out[n] = 0;
            return 1;
        }
        u8 b = stream_get(s);
		out[n++] = b;
    }
	out[n] = 0;
	return 0;
}

typedef enum
{
	k_EDescTagFunc = 0,
	k_EDescTagTable = 1,
	k_EDescTagMemory = 2,
	k_EDescTagGlobal = 3
} k_EDescTag;

static const char *desc_tag_strings[] = {
	"func", "table", "memory", "global", NULL
};

void stream_decode_limits(stream_t* s, u32* min, u32* max, u8 *max_present)
{
	*min = *max = 0;
	u8 present = stream_get(s);
	*min = stream_decode_leb_128(s);
	if (present)
	{
		*max = stream_decode_leb_128(s);
	}
	if (max_present)
		*max_present = present;
}

void stream_decode_value_type(stream_t *s, k_EValueType *type)
{
	*type = (k_EValueType)stream_get(s);
	//TODO: add check whether it's a valid type
}

void stream_decode_ref_type(stream_t *s, k_EValueType *type)
{
	*type = (k_EValueType)stream_get(s);
	assert(*type == 0x70 || *type == 0x6f);
}

void stream_decode_mut(stream_t* s, bool *mut)
{
	u8 b = stream_get(s);
	assert(b == 0 || b == 1);
	*mut = b != 0;
}

typedef struct
{
	bool mut;
	k_EValueType valtype;
} globaltype_t;

void stream_decode_globaltype(stream_t *s, globaltype_t *gt)
{
    stream_decode_value_type(s, &gt->valtype);
    stream_decode_mut(s, &gt->mut);
}

typedef struct
{
} instruction_t;

typedef struct
{
} expression_t;

void stream_decode_instruction(stream_t *s, instruction_t *instr)
{
}

void stream_decode_expression(stream_t *s, expression_t *gt)
{
}

void read_section_global(stream_t *s)
{
    u32 n = stream_decode_leb_128(s);
    for (size_t i = 0; i < n; ++i)
    {
        globaltype_t gt;
        stream_decode_globaltype(s, &gt);
		printf("%d: mutable %d, valtype %s\n", i, gt.mut, value_type_to_string(gt.valtype));
    }
}

void read_section_function(stream_t *s)
{
    u32 n = stream_decode_leb_128(s);
    for (size_t i = 0; i < n; ++i)
    {
		u32 typeidx = stream_decode_leb_128(s);
		printf("%d -> %d\n", i, typeidx);
    }
}

void read_section_import(stream_t *s)
{
    size_t numimports = stream_get(s);
    printf("numimports=%d\n", numimports);
    for (size_t i = 0; i < numimports; ++i)
    {
		printf("import %d\n", i);
		char module[32] = {0};
		read_string(s, module, sizeof(module));
		char name[32] = {0};
		read_string(s, name, sizeof(name));
		u8 desc_tag = stream_get(s);
		printf("module = %s, name = %s, desc_tag = %s\n", module, name, desc_tag_strings[desc_tag]);
		switch (desc_tag)
		{

			case k_EDescTagMemory:
			{
				u32 min, max;
				u8 present;
				stream_decode_limits(s, &min, &max, &present);
			}
			break;

			case k_EDescTagTable:
			{
				k_EValueType reftype;
				u32 min, max;
				u8 present;
				stream_decode_ref_type(s, &reftype);
				stream_decode_limits(s, &min, &max, &present);
				printf("min = %d, max = %d, reftype = %x\n", min, max, reftype);
			}
			break;

			case k_EDescTagFunc:
			{
				u32 typeidx = stream_decode_leb_128(s);
				printf("typeidx = %d\n", typeidx);
			}
			break;

			case k_EDescTagGlobal:
			{
				globaltype_t gt;
				stream_decode_globaltype(s, &gt);
				printf("mutable %d, value type = %s\n", gt.mut, value_type_to_string(gt.valtype));
			}
			break;

			default:
				printf("Unhandled description tag %s\n", desc_tag_strings[desc_tag]);
				exit(-1);
				break;
		}
	}
}

typedef struct
{
	int id;
	void (*fn)(stream_t*);
} section_id_t;

static const section_id_t section_ids[] = {
	{k_ESectionIdType, read_section_type},
	{k_ESectionIdImport, read_section_import},
	{k_ESectionIdFunction, read_section_function},
	{k_ESectionIdGlobal, read_section_global},
	{0, NULL}
};

void read_section(stream_t *s)
{
	u8 type = stream_get(s);
	u32 sz = stream_decode_leb_128(s);
	const char* section_id_string = section_id_strings[type];
	printf("Section %s (%x bytes)\n", section_id_string, sz);

	bool fnd = false;
    for (size_t i = 0; section_ids[i].fn; ++i)
    {
        if (section_ids[i].id == type)
        {
            section_ids[i].fn(s);
            fnd = true;
            break;
        }
    }
    if (!fnd)
    {
			printf("Unhandled section %s\n", section_id_string);
    }
}

int main(int argc, char **argv)
{
	const char *file = "test.wasm";
	if(argc > 1)
		file = argv[1];
	
	size_t sz = 0;
	u8* buf = read_file(file, &sz);
	if (!buf)
	{
		printf("Could not open '%s'\n", file);
		return 0;
	}
	stream_t s = {0};
	stream_init(&s, buf, sz);

	char magic[4];
	stream_read(&s, magic, sizeof(magic));

	u32 version;
	STREAM_READ(s, version);

	printf("magic = %c, %c, %c, %c\n", magic[0], magic[1], magic[2], magic[3]);
	printf("version = %d\n", version);

	read_section(&s);
	read_section(&s);
	read_section(&s);
	read_section(&s);
	
	free(buf);
	return 0;
}

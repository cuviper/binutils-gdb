/* Opcode table for the TXVU
   Copyright 1998 Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ansidecl.h"
#include "dis-asm.h"
#include "opcode/txvu.h"

#ifndef NULL
#define NULL 0
#endif

#if defined (__STDC__) || defined (ALMOST_STDC)
#define XCONCAT2(a,b)	a##b
#else
#define XCONCAT2(a,b)	a/**/b
#endif
#define CONCAT2(a,b)	XCONCAT2(a,b)

/* ??? One can argue it's preferable to have the PARSE_FN support in tc-vxvu.c
   and the PRINT_FN support in txvu-dis.c.  For this project I like having
   them all in one place.  */

#define PARSE_FN(fn) \
static long CONCAT2 (parse_,fn) \
     PARAMS ((char **, const char **));
#define INSERT_FN(fn) \
static TXVU_INSN CONCAT2 (insert_,fn) \
     PARAMS ((TXVU_INSN, const struct txvu_operand *, \
	      int, long, const char **))
#define EXTRACT_FN(fn) \
static long CONCAT2 (extract_,fn) \
     PARAMS ((TXVU_INSN, const struct txvu_operand *, \
	      int, int *))
#define PRINT_FN(fn) \
static void CONCAT2 (print_,fn) \
     PARAMS ((disassemble_info *, TXVU_INSN, long));

PARSE_FN (dotdest);
INSERT_FN (dotdest);
EXTRACT_FN (dotdest);
PRINT_FN (dotdest);

PARSE_FN (vfreg);
PRINT_FN (vfreg);

/* Various types of ARC operands, including insn suffixes.

   Fields are:

   BITS SHIFT FLAGS PARSE_FN INSERT_FN EXTRACT_FN PRINT_FN

   Operand values are 128 + table index.  This allows ASCII chars to be
   included in the syntax spec.  */

const struct txvu_operand txvu_operands[] =
{
/* place holder (??? not sure if needed) */
#define UNUSED 128
  { 0 },

/* Destination indicator, with leading '.'.  */
#define DOTDEST (UNUSED + 1)
  { 4, TXVU_SHIFT_DEST, TXVU_OPERAND_SUFFIX,
      parse_dotdest, insert_dotdest, extract_dotdest, print_dotdest },

/* ft reg */
#define FTREG (DOTDEST + 1)
  { 5, TXVU_SHIFT_FTREG, 0, parse_vfreg, 0, 0, print_vfreg },

/* fs reg */
#define FSREG (FTREG + 1)
  { 5, TXVU_SHIFT_FSREG, 0, parse_vfreg, 0, 0, print_vfreg },

/* fd reg */
#define FDREG (FSREG + 1)
  { 5, TXVU_SHIFT_FDREG, 0, parse_vfreg, 0, 0, print_vfreg },

/* end of list place holder */
  { 0 }
};

/* Macros to put a field's value into the right place.  */
#define FT(x) (((x) & TXVU_MASK_VFREG) << TXVU_SHIFT_FTREG)
#define FS(x) (((x) & TXVU_MASK_VFREG) << TXVU_SHIFT_FSREG)
#define FD(x) (((x) & TXVU_MASK_VFREG) << TXVU_SHIFT_FDREG)
#define R(x,b,m) (((x) & (m)) << (b))	/* value X, mask M, at bit B */

/* TXVU instructions.
   [??? some of these comments are left over from the ARC port from which
   this code is borrowed, delete in time]

   Longer versions of insns must appear before shorter ones (if gas sees
   "lsr r2,r3,1" when it's parsing "lsr %a,%b" it will think the ",1" is
   junk).  This isn't necessary for `ld' because of the trailing ']'.

   Instructions that are really macros based on other insns must appear
   before the real insn so they're chosen when disassembling.  Eg: The `mov'
   insn is really the `and' insn.

   This table is best viewed on a wide screen (161 columns).  I'd prefer to
   keep it this way.  The rest of the file, however, should be viewable on an
   80 column terminal.  */

/* ??? This table also includes macros: asl, lsl, and mov.  The ppc port has
   a more general facility for dealing with macros which could be used if
   we need to.  */

/* These tables can't be `const' because members `next_asm' and `next_dis' are
   computed at run-time.  We could split this into two, as that would put the
   constant stuff into a readonly section.  */

struct txvu_opcode txvu_upper_opcodes[] = {

  /* Macros appear first.  */
  /* ??? Any aliases?  */

  /* The rest of these needn't be sorted, but it helps to find them if they
     are.  */
  { "abs", { DOTDEST, ' ', FTREG, FSREG }, 0xfe0001ff, 0x1fd, 0 },
  { "add", { DOTDEST, ' ', FDREG, FSREG, FTREG }, 0xfe00003f, 0x28, 0 },
};
const int txvu_upper_opcodes_count = sizeof (txvu_upper_opcodes) / sizeof (txvu_opcodes[0]);

struct txvu_opcode txvu_lower_opcodes[] = {

  /* Macros appear first.  */
  /* ??? Any aliases?  */

  /* The rest of these needn't be sorted, but it helps to find them if they
     are.  */
  { "waitp", { 0 }, 0xffffffff, 0x800007bf, 0 },
  { "waitq", { 0 }, 0xffffffff, 0x800003bf, 0 },
};
const int txvu_lower_opcodes_count = sizeof (txvu_lower_opcodes) / sizeof (txvu_opcodes[0]);

/* Indexed by first letter of opcode.  Points to chain of opcodes with same
   first letter.  */
/* ??? One can certainly use a better hash.  Later.  */
static struct txvu_opcode *upper_opcode_map[26 + 1];
static struct txvu_opcode *lower_opcode_map[26 + 1];

/* Indexed by insn code.  Points to chain of opcodes with same insn code.  */
static struct txvu_opcode *upper_icode_map[64];
static struct txvu_opcode *lower_icode_map[64];

/* Initialize any tables that need it.
   Must be called once at start up (or when first needed).

   FLAGS is currently unused but is intended to control initialization.  */

void
txvu_opcode_init_tables (flags)
     int flags;
{
  static int init_p = 0;

  /* We may be intentionally called more than once (for example gdb will call
     us each time the user switches cpu).  These tables only need to be init'd
     once though.  */
  /* ??? We can remove the need for txvu_opcode_supported by taking it into
     account here, but I'm not sure I want to do that yet (if ever).  */
  if (!init_p)
    {
      int i,n;

      memset (upper_opcode_map, 0, sizeof (upper_opcode_map));
      memset (upper_icode_map, 0, sizeof (upper_icode_map));

      /* Scan the table backwards so macros appear at the front.  */
      for (i = txvu_upper_opcodes_count - 1; i >= 0; --i)
	{
	  int opcode_hash = TXVU_HASH_UPPER_OPCODE (txvu_upper_opcodes[i].mnemonic);
	  int icode_hash = TXVU_HASH_UPPER_ICODE (txvu_upper_opcodes[i].value);

	  txvu_upper_opcodes[i].next_asm = upper_opcode_map[opcode_hash];
	  upper_opcode_map[opcode_hash] = &txvu_upper_opcodes[i];

	  txvu_upper_opcodes[i].next_dis = upper_icode_map[icode_hash];
	  upper_icode_map[icode_hash] = &txvu_upper_opcodes[i];
	}

      memset (lower_opcode_map, 0, sizeof (lower_opcode_map));
      memset (lower_icode_map, 0, sizeof (lower_icode_map));

      /* Scan the table backwards so macros appear at the front.  */
      for (i = txvu_lower_opcodes_count - 1; i >= 0; --i)
	{
	  int opcode_hash = TXVU_HASH_LOWER_OPCODE (txvu_lower_opcodes[i].mnemonic);
	  int icode_hash = TXVU_HASH_LOWER_ICODE (txvu_lower_opcodes[i].value);

	  txvu_lower_opcodes[i].next_asm = lower_opcode_map[opcode_hash];
	  lower_opcode_map[opcode_hash] = &txvu_lower_opcodes[i];

	  txvu_lower_opcodes[i].next_dis = lower_icode_map[icode_hash];
	  lower_icode_map[icode_hash] = &txvu_lower_opcodes[i];
	}

      init_p = 1;
    }
}

/* Return the first insn in the chain for assembling upper INSN.  */

const struct txvu_opcode *
txvu_upper_opcode_lookup_asm (insn)
     const char *insn;
{
  return upper_opcode_map[TXVU_HASH_UPPER_OPCODE (insn)];
}

/* Return the first insn in the chain for assembling lower INSN.  */

const struct txvu_opcode *
txvu_lower_opcode_lookup_asm (insn)
     const char *insn;
{
  return lower_opcode_map[TXVU_HASH_LOWER_OPCODE (insn)];
}

/* Return the first insn in the chain for disassembling upper INSN.  */

const struct txvu_opcode *
txvu_upper_opcode_lookup_dis (insn)
     TXVU_INSN insn;
{
  return upper_icode_map[TXVU_HASH_UPPER_ICODE (insn)];
}

/* Return the first insn in the chain for disassembling lower INSN.  */

const struct txvu_opcode *
txvu_lower_opcode_lookup_dis (insn)
     TXVU_INSN insn;
{
  return lower_icode_map[TXVU_HASH_LOWER_ICODE (insn)];
}

/* Value of DEST in use.
   Each of the registers must specify the same value as the opcode.
   ??? Perhaps remove the duplication?  */
static int dest;

/* Init fns.
   These are called before doing each of the respective activities.  */

/* Called by the assembler before parsing an instruction.  */

void
txvu_opcode_init_parse ()
{
  dest = -1;
}

/* Called by the disassembler before printing an instruction.  */

void
txvu_opcode_init_print ()
{
  dest = -1;
}

/* Destination choice support.
   The "dest" string selects any combination of x,y,z,w.
   [The letters are ordered that way to follow the manual's style.]  */

/* Parse a `dest' spec.
   Return the found value.
   *PSTR is set to the character that terminated the parsing.
   It is up to the caller to do any error checking.  */

static long
parse_dest (pstr)
     char **pstr;
{
  long dest = 0;

  while (**pstr)
    {
      switch (**pstr)
	{
	case 'x' : case 'X' : dest |= TXVU_DEST_X; break;
	case 'y' : case 'Y' : dest |= TXVU_DEST_Y; break;
	case 'z' : case 'Z' : dest |= TXVU_DEST_Z; break;
	case 'w' : case 'W' : dest |= TXVU_DEST_W; break;
	default : return dest;
	}
      ++*pstr;
    }

  return dest;
}

static long
parse_dotdest (pstr, errmsg)
     char **pstr;
     const char **errmsg;
{
  long dest;

  if (**pstr != '.')
    {
      *errmsg = "missing `.'";
      return 0;
    }

  ++*pstr;
  dest = parse_dest (pstr);
  if (dest == 0 || isalnum (**pstr))
    {
      *errmsg = "invalid `dest'";
      return 0;
    }
  *errmsg = NULL;
  return dest;
}

static TXVU_INSN
insert_dotdest (insn, operand, mods, value, errmsg)
     TXVU_INSN insn;
     const struct txvu_operand *operand;
     int mods;
     long value;
     const char **errmsg;
{
  /* Record the DEST value in use so the register parser can use it.  */
  dest = value;
  if (errmsg)
    *errmsg = NULL;
  return insn |= value << operand->shift;
}

static long
extract_dotdest (insn, operand, mods, pinvalid)
     TXVU_INSN insn;
     const struct txvu_operand *operand;
     int mods;
     int *pinvalid;
{
  /* Record the DEST value in use so the register printer can use it.  */
  dest = (insn >> operand->shift) & ((1 << operand->bits) - 1);
  return dest;
}

static void
print_dest (info, insn, value)
     disassemble_info *info;
     TXVU_INSN insn;
     long value;
{
  if (value & TXVU_DEST_X)
    (*info->fprintf_func) (info->stream, "x");
  if (value & TXVU_DEST_Y)
    (*info->fprintf_func) (info->stream, "y");
  if (value & TXVU_DEST_Z)
    (*info->fprintf_func) (info->stream, "z");
  if (value & TXVU_DEST_W)
    (*info->fprintf_func) (info->stream, "w");
}

static void
print_dotdest (info, insn, value)
     disassemble_info *info;
     TXVU_INSN insn;
     long value;
{
  (*info->fprintf_func) (info->stream, ".");
  print_dest (info, insn, value);
}

static long
parse_vfreg (pstr, errmsg)
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;
  int reg_dest;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'f')
    {
      *errmsg = "unknown register";
      return 0;
    }

  /* FIXME: quick hack until the framework works.  */
  start = str = str + 2;
  while (*str && isdigit (*str))
    ++str;
  reg = atoi (start);
  reg_dest = parse_dest (&str);
  if (reg_dest == 0 || isalnum (*str))
    {
      *errmsg = "invalid `dest'";
      return 0;
    }
  if (reg_dest != dest)
    {
      *errmsg = "register `dest' does not match instruction `dest'";
      return 0;
    }
  *pstr = str;
  *errmsg = NULL;
  return reg;
}

static void
print_vfreg (info, insn, value)
     disassemble_info *info;
     TXVU_INSN insn;
     long value;
{
  (*info->fprintf_func) (info->stream, "vf%ld", value);
  print_dest (info, insn, dest);
}

/* irfunc.c v0.4.1 (c) 1998-99 Tom Wheeley <tomw@tsys.demon.co.uk> */
/* this code is placed under the LGPL, see www.gnu.org for info    */

/*
 * irfunc.c, infrared functions (Irman specific)
 */

#include "ir.h"

/* generic function for ir_get_code() and ir_poll_code() */
static unsigned char *ir_read_code(unsigned long timeout);

/* converts a single hex character to an integer */
static int ir_hex_to_int(unsigned char hex);

/* flag to enable use of higher level functions */
static int ir_enabled = 0;

/* output hex digits */
static char ir_hexdigit[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

/* this function is used by both ir_get_code() and ir_poll_code(),
 * the difference being in the timeout for the first piece of data.
 * we also have a short timeout whatever for the remaining five bytes,
 * in case computer and Irman get out of sync we can just raise an error
 * and get back to normal life.
 *
 * note esp. that these functions return a pointer to statically defined
 * data.  In the forseeable usage of LIBIRMAN this seems the easiest way.
 */

static unsigned char *ir_read_code(unsigned long timeout)
{
	static unsigned char codebuf[IR_MAX_CODE_LEN]; 
	int i, datum;

	datum = ir_read_char(timeout);
	if (datum < 0)
		return NULL;

	codebuf[0] = (unsigned char) datum;

	for (i = 1; i < ircfg.codelen; i++)
	{
		datum = ir_read_char(IR_POLL_TIMEOUT);
		if (datum < 0)
		{
			return NULL;
		}
		else
		{
			codebuf[i] = (unsigned char) datum;
		}
	}

	return codebuf;
}

unsigned char *ir_get_code(void)
{
	/* well dodgy choice of error here...! */
	if (!ir_enabled)
	{
		errno = ENXIO;
		return NULL;
	}

	return ir_read_code(IR_BLOCKING);
}

unsigned char *ir_poll_code(void)
{
	if (!ir_enabled)
	{
		errno = ENXIO;
		return NULL;
	}

	return ir_read_code(0);
}

/* checks to see if a piece of text is a valid ir code (1 = yes) */
int ir_valid_code(char *text)
{
	char *c;

	if (strlen(text) != ircfg.codelen * 2)
		return 0;

	for (c = text; *c; c++)
		if (!isxdigit(*c))
			return 0;

	return 1;
}

char *ir_code_to_text(unsigned char *code)
{
	static char text[2 * IR_MAX_CODE_LEN + 1];
	int i;
	char *j;

	j = text;
	for (i = 0; i < ircfg.codelen; i++)
	{
		*j++ = ir_hexdigit[(code[i] >> 4) & 0x0f];
		*j++ = ir_hexdigit[code[i] & 0x0f];
	}
	*j = '\0';

	return text;
}

static int ir_hex_to_int(unsigned char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';

	hex = tolower(hex);
	if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;

	/* error! */
	return 0;
}

unsigned char *ir_text_to_code(char *text)
{
	static char code[IR_MAX_CODE_LEN];
	int i;
	char *j;

	j = text;
	for (i = 0; i < ircfg.codelen; i++)
	{
		if (!j[0] || !j[1])
		{
			break;
		}
		code[i] = (ir_hex_to_int(*j++) << 4) & 0xf0;
		code[i] += (ir_hex_to_int(*j++)) & 0x0f;
	}

	/* if string isn't long enough, pad with zeros. This is (marginally)
	 * better than the leaving in the remains of the last conversion
	 */
	for (; i < ircfg.codelen; i++)
		code[i] = '\0';

	return code;
}

/* this function should never be called, but maybe someone wants to manually
 * open the ir channel, then use the higher level functions.  If you use this
 * then you deserve any problems you get!  It is only here because I don't
 * believe in unneccesary restrictions.
 */
void ir_set_enabled(int val)
{
	ir_enabled = val;
}

/* end of irfunc.c */

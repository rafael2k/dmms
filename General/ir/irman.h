/* irman.h v0.4.1 (c) 1998-99 Tom Wheeley <tomw@tsys.demon.co.uk> */
/* this code is placed under the LGPL, see www.gnu.org for info   */

#ifndef IRMAN_H
#define IRMAN_H

/* number of bytes sent back from the IR interface */
#define IR_MAX_CODE_LEN 6 /* I don't know of any remotes that use code lengths
			     longer than 6, if you do, please change this. */

/* timing details. we have `SSEC' instead of `SEC' due to clashes with
 * some (ie Solaris) <time.h> definitions  */

#define SSEC	* 1000000
#define MSEC	* 1000
#define USEC

/* Assuming LONG_MAX to be 2,000,000,000, we have a maximum timeout of
 * approx 2,000s, ie over half an hour.  Plenty! (I should hope)
 */

/* time we allow the port to sort itself out with */
#define IR_POWER_ON_LATENCY		(10 MSEC)
/* gap between sending 'I' and 'R' */
#define IR_HANDSHAKE_GAP		(500 USEC)

/* successive initial garbage characters should not be more than this apart */
#define IR_GARBAGE_TIMEOUT		(50 MSEC)
/* letters 'O' and 'K' should arrive within this */
#define IR_HANDSHAKE_TIMEOUT		(2 SSEC)
/* successive bytes of an ir pseudocode should arrive within this time limit */
#define IR_POLL_TIMEOUT			(1 MSEC)

/* timeout for blocking IO */
#define IR_BLOCKING			(-1)

/* return from ir_get_command() on error */
#define IR_CMD_ERROR			-1
#define IR_CMD_UNKNOWN			0

/* size of hash table in ircmd.c.  must be prime */
#define IR_HT_SIZE			271

/* size of string to hold default Irman port name, eg /dev/ttyS0 */
#define IR_PORTNAME_LEN			127

/* filename for system irmanrc */
#ifndef IR_SYSCONF_DIR
#define IR_SYSCONF_DIR			"/etc"
#endif
#define IR_SYSTEM_IRMANRC		IR_SYSCONF_DIR "/irman.conf"
/* filename for users irmanrc */
#define IR_USER_IRMANRC			".irmanrc"

/* messages printed by ir_ask_for_code() */
#define IR_ASK_GREETING	"please press the button for %s\n"
#define IR_ASK_REPEAT	"press %s again, to be sure...\n"
#define IR_ASK_OK	"Thankyou.\n"
#define IR_ASK_NOMATCH	"The two codes do not match.  "

#define ir_code_to_name(c)	ir_text_to_name(ir_code_to_text(c))
#define ir_name_to_code(n)	ir_text_to_code(ir_name_to_text(n))

/*
 * Function prototypes
 */

/* high level: ircmd.c */
int ir_init_commands(char *rcname, int warn);
char *ir_text_to_name(char *text);
char *ir_name_to_text(char *name);
int ir_bind(char *name, char *text);
int ir_alias(char *newname, char *name);
int ir_register_command(char *name, int command);
int ir_remove_command(char *name);
int ir_get_command(void);
int ir_poll_command(void);
void ir_free_commands(void);
unsigned char *ir_ask_for_code(char *name, int display);
void ir_set_cmd_enabled(int val);
char *ir_default_portname(void);

/* mid level: irfunc.c */
int ir_init(char *filename);
int ir_finish(void);
unsigned char *ir_get_code(void);
unsigned char *ir_poll_code(void);
int ir_valid_code(char *text);
char *ir_code_to_text(unsigned char *code);
unsigned char *ir_text_to_code(char *text);
void ir_set_enabled(int val);

/* low level: irio.c */
int ir_open_port(char *filename);
int ir_get_portfd(void);
int ir_close_port(void);
int ir_write_char(unsigned char data);
int ir_read_char(long timeout);
void ir_clear_buffer(void);
void ir_usleep(unsigned long usec);

/* purely internal stuff */

#ifdef __IR

#else

#endif /* __IR */

#endif /* IRMAN_H */

/* end of irman.h */

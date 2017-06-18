/* irio.c v0.4.1 (c) 1998-99 Tom Wheeley <tomw@tsys.demon.co.uk> */
/* this code is placed under the LGPL, see www.gnu.org for info  */

/*
 * irio.c, Irman infrared controller interface
 */

#include "ir.h"

/* wrapper for ir_close_port() for use by atexit() */
/* static RETSIGTYPE ir_close_port_on_exit(void) { (void) ir_close_port(); } */

static int portfd = 0;
static int portflags = 0;
static int oldflags = 0;
static struct termios oldterm;
static struct termios portterm;

/*
 * Note regarding terminal settings.
 *
 * These work on my system.  I am quite confident they will work on other
 * systems.  The termios setup code is originally from another program
 * designed to talk to a serial device (casio diary) written by someone who I
 * can't remember but I presume they knew what they were doing.
 *
 * More information on Unix serial port programming can be obtained from
 *   http://www.easysw.com/~mike/serial/index.html
 *
 * addendum.  I tried for quite a while to get this all to work on a Sun
 * Ultra 1, but it didn't :(  All the attempts are under ifdef SUNATTEMPT.
 */

/*
 * A future avenue might be to set the settings to 0 and work from there.
 * Also check for other names for the serial port as in IRIX -- maybe
 * Solaris does that (I tried ttya)
 */

int ir_open_port(char *filename)
{
	int parnum = 0;

#ifdef SUNATTEMPT
	int hand = TIOCM_DTR | TIOCM_RTS;

#endif
	int baudrate = B9600;

	/* get a file descriptor */
	if ((portfd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
	{
		return -1;
	}

	/* check to see that the file is a terminal */
	if (!isatty(portfd))
	{
		return -1;
	}

	/* get port attributes, store in oldterm */
	if (tcgetattr(portfd, &oldterm) < 0)
	{
		return -1;
	}

	/* get port flags, save in oldflags */
	if ((oldflags = fcntl(portfd, F_GETFL)) < 0)
	{
		return -1;
	}

	/* now we have read the old attributes for the port, we can restore them
	 * upon exit. if we had done this bfore, and exited beore reading in the
	 * old attributes, we would have overwritten the old settings with zeros.  
	 *
	 * this way, if we do exit before we get here, we simply rely on the OS closing
	 * the port for us, which is fine as we haven't changed anything yet.
	 */

/*  atexit(ir_close_port_on_exit); */

	/* copy old attrs into new structure */
	portterm = oldterm;
	portflags = oldflags;

	/* remove old parity setting, size and stop setting */
	portterm.c_cflag &= ~PARENB;
	portterm.c_cflag &= ~PARODD;
	portterm.c_cflag &= ~CSTOPB;
	portterm.c_cflag &= ~CSIZE;

	/* set character size, stop bits and parity */
	portterm.c_cflag |= CS8;
	portterm.c_cflag |= parnum;

	/* enable receiver, and don't change ownership */
	portterm.c_cflag |= CREAD | CLOCAL;

	/* disable flow control */
#ifdef CNEW_RTSCTS
	portterm.c_cflag &= ~CNEW_RTSCTS;
#else
#ifdef CRTSCTS
	portterm.c_cflag &= ~CRTSCTS;
#endif
#ifdef CRTSXOFF
	portterm.c_cflag &= ~CRTSXOFF;
#endif
#endif

	/* read characters immediately in non-canonical mode */
	/* Thanks to Bill Ryder, <bryder@sgi.com> */
	portterm.c_cc[VMIN] = 1;
	portterm.c_cc[VTIME] = 1;

	/* set the input and output baud rate */
	cfsetispeed(&portterm, baudrate);
	cfsetospeed(&portterm, baudrate);

	/* set non-canonical mode (we don't want any fancy terminal processing!) */
	portterm.c_lflag = 0;

	/* Ignore breaks and make terminal raw and dumb. */
	portterm.c_iflag = 0;
	portterm.c_iflag |= IGNBRK;
	portterm.c_oflag &= ~OPOST;

	/* set the input and output baud rate */
	cfsetispeed(&portterm, baudrate);
	cfsetospeed(&portterm, baudrate);

	/* now clean the serial line and activate the new settings */
	tcflush(portfd, TCIOFLUSH);
	if (tcsetattr(portfd, TCSANOW, &portterm) < 0)
	{
		return -1;
	}

	/* set non-blocking */
	if (fcntl(portfd, F_SETFL, (portflags |= O_NONBLOCK)) < 0)
	{
		return -1;
	}

#ifdef SUNATTEMPT
	/* raise the control lines to power the unit */
	if (ioctl(portfd, TIOCMSET, &hand) < 0)
	{
		printf("ioctl error\n");
		return -1;
	}
#endif

	/* wait a little while for everything to settle through */
	ir_usleep(IR_POWER_ON_LATENCY);

	/* it has been suggested that ir_open_port() returns the portfd, however
	 * this would mean breaking the current convention where returning 0 means
	 * success.  Also it might encourage people to play with the fd, with is
	 * probably a bad idea unless you know what you are doing.
	 * however for people who _do_ know what they are doing --
	 *    use ir_get_portfd()
	 * Thanks to mumble, <mumble@mumble.gr> for the suggestion. (sorry I forgot
	 * your name/address -- email me and I'll put it in!)
	 */
	return 0;
}

/* return the portfd for the serial port.  don't mess it up, please.
 * returns 0 if port not open
 */
int ir_get_portfd(void)
{
	return portfd;
}

/* close the port, restoring old settings */

int ir_close_port(void)
{
	int retval = 0;

	if (!portfd)
	{			/* already closed */
		errno = EBADF;
		return -1;
	}

	/* restore old settings */
	if (tcsetattr(portfd, TCSADRAIN, &oldterm) < 0)
	{
		retval = -1;
	}

	if (fcntl(portfd, F_SETFL, oldflags) < 0)
	{
		retval = -1;
	}

	close(portfd);
	portfd = 0;

	return retval;
}

/* write a character.  nothing interesting happens here */

int ir_write_char(unsigned char data)
{
	if (write(portfd, &data, 1) != 1)
		return -1;
	else
	{
#ifdef DEBUG_COMM
		printf("{%02x}", data);
		fflush(stdout);
#endif
		return 0;
	}
}

/* read a character, with a timeout.
 * timeout < 0        -  block indefinitely
 * timeout = 0  -  return immediately
 * timeout > 0  -  timeout after `timeout' microseconds
 *                 use the nice macros in irman.h to define sec, msec, usec
 */

int ir_read_char(long timeout)
{
	unsigned char rdchar;
	int ok;
	fd_set rdfds;
	struct timeval tv;

	FD_ZERO(&rdfds);
	FD_SET(portfd, &rdfds);

	/* block until something to read or timeout occurs.  select() is damn cool */

	if (timeout < 0)
	{
		ok = select(portfd + 1, &rdfds, NULL, NULL, NULL);
	}
	else
	{
		tv.tv_sec = timeout / 1000000;
		tv.tv_usec = (timeout % 1000000);
		ok = select(portfd + 1, &rdfds, NULL, NULL, &tv);
	}

	if (ok > 0)
	{
		ok = read(portfd, &rdchar, 1);
		if (ok == 0)
		{
			return EOF;
		}
#ifdef COMM_DEBUG
		printf("[%02x]", rdchar);
		fflush(stdout);
#endif
		return rdchar;
	}
	else if (ok < 0)
	{
		return EOF - 1;
	}
	else
	{
		errno = ETIMEDOUT;
		return EOF - 1;
	}

	return 0;
}

/* just ir_about the only function where we don't care ir_about errors! */

void ir_clear_buffer(void)
{
	while (ir_read_char(IR_GARBAGE_TIMEOUT) >= 0)
		;
}

/* some systems have it, some systems don't.  This just makes life easier,
 * hence I have left this function visible (also for irfunc.c)
 */

void ir_usleep(unsigned long usec)
{
	struct timeval tv;

	tv.tv_sec = usec / 1000000;
	tv.tv_usec = (usec % 1000000);
	(void) select(0, NULL, NULL, NULL, &tv);
}

/* end of irio.c */

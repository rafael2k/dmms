/*
 * Copyright (C) 1999-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 * Licensed under GNU LGPL version 2.
 *
 */


char* xmms_charset_get_current(void);
char* xmms_charset_convert(const char *string, size_t insize, char *from, char *to);
char* xmms_charset_to_utf8(const char *string);
char* xmms_charset_from_utf8(const char *string);




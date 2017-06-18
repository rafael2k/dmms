/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2004  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2000-2004  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "xmms.h"
#include "xmms_logo.xpm"

/*
 * The different sections are kept in alphabetical order
 */

static const char *credit_text[] =
{N_("Main Programming:") ,
	N_("Peter Alm"),
	NULL,
 N_("Additional Programming:"),
	/* I18N: UTF-8 Translation: "H\303\245vard Kv\303\245len" */
	N_("Haavard Kvaalen"),
	N_("Derrik Pates"),
	NULL,
 N_("With Additional Help:"),
	N_("Tony Arcieri"),
	N_("Sean Atkinson"),
	N_("Jorn Baayen"),
	N_("James M. Cape"),
	N_("Anders Carlsson (effect plugins)"),
	N_("Chun-Chung Chen (xfont patch)"),
	N_("Tim Ferguson (joystick plugin)"),
	N_("Ben Gertzfield"),
	N_("Vesa Halttunen"),
	N_("Logan Hanks"),
	N_("Eric L. Hernes (FreeBSD patches)"),
	N_("Ville Herva"),
 	N_("Ian 'Hixie' Hickson"),
	N_("higway (MMX)"),
	N_("Michael Hipp and others (MPG123 engine)"),
	/* I18N: UTF-8 translation: "Olle H\303\244lln\303\244s ..." */
	N_("Olle Hallnas (compiling fixes)"),
	/* I18N: UTF-8 translation: "Matti H\303\244m\303\244l\303\244inen" */
	N_("Matti Hamalainen"),
	N_("David Jacoby"),
	N_("Osamu Kayasono (3DNow!/MMX)"),
	N_("Lyle B Kempler"),
	N_("J. Nick Koston (MikMod plugin)"),
	N_("Aaron Lehmann"),
	N_("Johan Levin (echo + stereo plugin)"),
	N_("Eric Lindvall"),
	N_("Colin Marquardt"),
	N_("Willem Monsuwe"),
	N_("Heikki Orsila"),
	N_("Gian-Carlo Pascutto"),
	N_("John Riddoch (Solaris plugin)"),
	N_("Josip Rodin"),
	N_("Pablo Saratxaga (i18n)"),
	N_("Carl van Schaik (pro logic plugin)"),
	/* I18N: UTF-8 translation: "J\303\266rg Schuler" */
	N_("Joerg Schuler"),
	N_("Charles Sielski (irman plugin)"),
	N_("Espen Skoglund"),
	N_("Matthieu Sozeau (ALSA plugin)"),
	N_("Kimura Takuhiro (3DNow!)"),
	N_("Zinx Verituse"),
	N_("Ryan Weaver (RPMs among other things)"),
	N_("Chris Wilson"),
	N_("Dave Yearke"),
	N_("Stephan K. Zitz"),
	NULL,
 N_("Default skin:"),
 	N_("Leonard \"Blayde\" Tan"),
	N_("Robin Sylvestre (Equalizer and Playlist)"),
	N_("Thomas Nilsson (New titles and cleanups)"),
	NULL,
 N_("Homepage and Graphics:"),
	N_("Thomas Nilsson"),
	NULL,
 N_("Support and Docs:"),
	/* I18N: UTF-8 translation: "Olle H\303\244lln\303\244s" */
	N_("Olle Hallnas"),
	NULL, NULL};

static const char *translators[] =
{
 N_("Afrikaans:"),
	/* I18N: UTF-8 translation: "Schalk W. Cronj\303\251" */
	N_("Schalk W. Cronje"), NULL,
 N_("Albanian:"),
	N_("Naim Daka"), NULL,
 N_("Azerbaijani:"),
	/* I18N: UTF-8 translation: "M\311\231tin \306\217mirov" */
	N_("Metin Amiroff"),
	/* I18N: UTF-8 translation: "Vasif \304\260smay\304\261lo\304\237lu" */
	N_("Vasif Ismailoglu"), NULL,
 N_("Basque:"),
	/* I18N: UTF-8 translation: "I\303\261igo Salvador Azurmendi" */
	N_("Inigo Salvador Azurmendi"), NULL,
 N_("Belarusian:"),
	N_("Smaliakou Zmicer"), NULL,
 N_("Bosnian:"),
	/* I18N: UTF-8 translation: "Amila Akagi\304\207" */
	N_("Amila Akagic"),
	N_("Grabovica Eldin"), N_("Vedran Ljubovic"), NULL,
 N_("Brazilian Portuguese:"),
	N_("Juan Carlos Castro y Castro"), NULL,
 N_("Bulgarian:"),
	N_("Boyan Ivanov"), N_("Yovko D. Lambrev"), NULL,
 N_("Catalan:"),
	N_("Albert Astals Cid"), N_("Quico Llach"), N_("Jordi Mallach"), NULL,
 N_("Chinese:"),
	N_("Chun-Chung Chen"), N_("Jouston Huang"), N_("Andrew Lee"),
	N_("Chih-Wei Huang"), N_("Shiyu Tang"), N_("Danny Zeng"), NULL,
 N_("Croatian:"),
	N_("Vlatko Kosturjak"), N_("Vladimir Vuksan"), NULL,
 N_("Czech:"),
	/* I18N: UTF-8 translation: "Vladim\303\255r Marek" */
	N_("Vladimir Marek"),
	N_("Radek Vybiral"), NULL,
 N_("Danish:"),
	N_("Nikolaj Berg Amondsen"), N_("Troels Liebe Bentsen"),
	N_("Kenneth Christiansen"), N_("Keld Simonsen"), NULL,
 N_("Dutch:"),
	N_("Jorn Baayen"), N_("Bart Coppens"), N_("Wilmer van der Gaast"),
	N_("Tom Laermans"), NULL,
 N_("Esperanto:"),
	N_("D. Dale Gulledge"), NULL,
 N_("Estonian:"),
	N_("Marek Laane"), NULL,
 N_("Finnish:"),
	N_("Thomas Backlund"), N_("Matias Griese"), NULL,
 N_("French:"),
	N_("Arnaud Boissinot"), N_("Eric Fernandez-Bellot"), NULL,
 N_("Galician:"),
	/* I18N: UTF-8 translation: "Alberto Garc\303\255a" */
	N_("Alberto Garcia"),
	/* I18N: UTF-8 translation: "David Fern\303\241ndez Vaamonde" */
	N_("David Fernandez Vaamonde"), NULL,
 N_("Georgian:"),
	N_("Aiet Kolkhi"), NULL,
 N_("German:"),
	N_("Colin Marquardt"), N_("Stefan Siegel"), NULL,
 N_("Greek:"),
	N_("Kyritsis Athanasios"), NULL,
 N_("Hungarian:"),
	N_("Arpad Biro"), NULL,
 N_("Indonesian:"),
	N_("Budi Rachmanto"), NULL,
 N_("Irish:"),
	N_("Alastair McKinstry"), NULL,
 N_("Italian:"),
	N_("Paolo Lorenzin"), N_("Daniele Pighin"), NULL,
 N_("Japanese:"),
	N_("Hiroshi Takekawa"), NULL,
 N_("Korean:"),
	N_("Jaegeum Choe"), N_("Sang-Jin Hwang"), N_("Byeong-Chan Kim"),
	N_("Man-Yong Lee"), NULL,
 N_("Lithuanian:"),
	N_("Gediminas Paulauskas"), NULL,
 N_("Latvian:"),
	/* I18N: UTF-8 translation: "Juris Kudi\305\206\305\241" */
	N_("Juris Kudins"),
	N_("Vitauts Stochka"), NULL,
 N_("Malay:"),
	N_("Muhammad Najmi Ahmad Zabidi"), NULL,
 N_("Norwegian:"),
	/* I18N: UTF-8 translation: "Andreas Bergstr\303\270m" */
	N_("Andreas Bergstrom"),
	N_("Terje Bjerkelia"), N_("Haavard Kvaalen"), N_("Roy-Magne Mo"),
	N_("Espen Skoglund"), NULL,
 N_("Polish:"),
	N_("Grzegorz Kowal"), NULL,
 N_("Portuguese:"),
	N_("Jorge Costa"), NULL,
 N_("Romanian:"),
	N_("Florin Grad"),
	/* I18N: UTF-8 translation: "Mi\305\237u Moldovan" */
	N_("Misu Moldovan"), NULL,
 N_("Russian:"),
	N_("Valek Filippov"), N_("Alexandr P. Kovalenko"),
	N_("Maxim Koshelev"), N_("Aleksey Smirnov"), NULL,
 N_("Serbian:"),
	N_("Tomislav Jankovic"), NULL,
 N_("Slovak:"),
	N_("Pavol Cvengros"), NULL,
 N_("Slovenian:"),
	N_("Tadej Panjtar"), N_("Tomas Hornocek"), N_("Jan Matis"), NULL,
 N_("Spanish:"),
	N_("Fabian Mandelbaum"), N_("Jordi Mallach"),
	/* I18N: UTF-8 translation: "Juan Manuel Garc\303\255a Molina" */
	N_("Juan Manuel Garcia Molina"), NULL,
 N_("Swedish:"),
	N_("David Hedbor"), N_("Olle Hallnas"),
	N_("Thomas Nilsson"), N_("Christian Rose"), N_("Fuad Sabanovic"), NULL,
 N_("Tajik:"),
	N_("Roger Kovacs"), N_("Dilshod Marupov"), NULL,
 N_("Thai:"),
	N_("Pramote Khuwijitjaru"), N_("Supphachoke Suntiwichaya"), NULL,
 N_("Turkish:"),
	N_("Nazmi Savga"),
	/* I18N: UTF-8 translation: "\303\226mer Fad\304\261l Usta" */
	N_("Omer Fadil Usta"), NULL,
 N_("Ukrainian:"),
	N_("Dmytro Koval'ov"), NULL,
 N_("Uzbek:"),
	N_("Mashrab Kuvatov"), NULL,
 N_("Vietnamese:"),
	N_("Trinh Minh Thanh"), NULL,
 N_("Walloon:"),
	N_("Lucyin Mahin"), N_("Pablo Saratxaga"), NULL,
 N_("Welsh:"),
	N_("Rhoslyn Prys"), NULL,
	NULL
};

static GtkWidget* generate_credit_list(const char *text[], gboolean sec_space)
{
	GtkWidget *clist, *scrollwin;
	int i = 0;

	clist = gtk_clist_new(2);

	while (text[i])
	{
		gchar *temp[2];
		guint row;
		
		temp[0] = gettext(text[i++]);
		temp[1] = gettext(text[i++]);
		row = gtk_clist_append(GTK_CLIST(clist), temp);
		gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		temp[0] = "";
		while (text[i])
		{
			temp[1] = gettext(text[i++]);
			row = gtk_clist_append(GTK_CLIST(clist), temp);
			gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		}
		i++;
		if (text[i] && sec_space)
		{
			temp[1] = "";
			row = gtk_clist_append(GTK_CLIST(clist), temp);
			gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		}
	}
	gtk_clist_columns_autosize(GTK_CLIST(clist));
	gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_RIGHT);
	
	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scrollwin), clist);
	gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 10);
	gtk_widget_set_usize(scrollwin, -1, 120);

	return scrollwin;
}

void show_about_window(void)
{
	static GtkWidget *about_window = NULL;
	static GdkPixmap *xmms_logo_pmap = NULL, *xmms_logo_mask = NULL;

	GtkWidget *about_vbox, *about_notebook;
	GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
	GtkWidget *about_credits_logo;
	GtkWidget *bbox, *close_btn;
	GtkWidget *label, *list;
	gchar *text;

	if (about_window)
		return;
	
	about_window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(about_window), _("About XMMS"));
	gtk_window_set_policy(GTK_WINDOW(about_window), FALSE, TRUE, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);
	gtk_signal_connect(GTK_OBJECT(about_window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_window);
	gtk_signal_connect(GTK_OBJECT(about_window), "key_press_event",
			   util_dialog_keypress_cb, NULL);
	gtk_widget_realize(about_window);
	
	about_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(about_window), about_vbox);
	
	if (!xmms_logo_pmap)
		xmms_logo_pmap =
			gdk_pixmap_create_from_xpm_d(about_window->window,
						     &xmms_logo_mask, NULL,
						     xmms_logo);
	
	about_credits_logo_box = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
			   FALSE, FALSE, 0);
	
	about_credits_logo_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
				  GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
			   about_credits_logo_frame, FALSE, FALSE, 0);
	
	about_credits_logo = gtk_pixmap_new(xmms_logo_pmap, xmms_logo_mask);
	gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
			  about_credits_logo);
	
	text = g_strdup_printf(_("XMMS %s - Cross platform multimedia player"),
			       VERSION);
	label = gtk_label_new(text);
	g_free(text);
	
	gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);
	
	label = gtk_label_new(_("Copyright (C) 1997-2004 4Front Technologies "
				"and The XMMS Team"));
	gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);
	
	about_notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);
	
	list = generate_credit_list(credit_text, TRUE);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
				 gtk_label_new(_("Credits")));

	list = generate_credit_list(translators, FALSE);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
				 gtk_label_new(_("Translators")));
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

	close_btn = gtk_button_new_with_label(_("Close"));
	gtk_signal_connect_object(GTK_OBJECT(close_btn), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(about_window));
	GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
	gtk_widget_grab_default(close_btn);

	gtk_widget_show_all(about_window);
}

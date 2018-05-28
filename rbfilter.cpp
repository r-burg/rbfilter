/*! \mainpage FilterCalcs

Copyright (c) 1971 .. 2016 Roger Burghall

The original of this code was written in Fortran for the Digital
Equipment Corporation PDP-12, beginning in the early 1970s, and then
ported to the BBC Model B, and extended to provide more of the 
functions described below. This port took place in the late 1980s. A second
port to Windows in the 1990's was abandoned in favour of a third port,
to a Linux PC, only begun in 2013.

No responsibility is accepted for the consequences of using this software,
and no guarantee is given for its correctness. It is necessary for the user
to understand what they are doing!

Copyright (C) 2013 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.


This program is intended to enable the user to design active or digital
filters, first in terms of T and q values and zero or one simple lag,
before carrying out circuit design to realise these stages as
either Sallen and Key or Rauch circuits, or as digital filters.

The intention is to permit the frequency and step responses to be
computed, the response to an arbitrary waveform (eventually; not implemented 
in version 1.0), and to design Sallen and Key or Rauch circuits, or transform
the filter to the z-domain. These functions were already implemented in the
BBC Model B version of the suite.

Thanks are due to Graham Watts for help with the mathematics of impulse
and step response functions.


Versions 0.1 to 0.51 RB
Developing code to design filters, compute frequency and time responses etc.
Takes specification via control window; returns information on graphics window and terminal window.

Version 0.6 June 2016 RB
Added circuit diagram window and code to show and hide diagrams.

Version 0.7 August 2016 RB
Normalise the cut-off frequency of the prototype filter (0.71: actually works!)

Version 0.8 August 2016 RB
Added a slider at the top of the graphics window.

Version 1.0 September 2016 RB
First usable version of this port.

*/

/*! \file rbfilter.cpp
rbfilter.cpp contains code to display windows to provide
a GUI for controlling the calculation of active filters
and in order to display results.

If porting to another OS this file is expected to need considerable modification.
It is intended that operating system dependant code should be kept in this file only.
*/


#include <gnome.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libgnomeui/libgnomeui.h>
#include <string.h>
#include "Drawing.h"
#include "Calcs.h"
#include <iostream>
#include <assert.h>


#define VERSION 1.0

#if SHOW_FILE_COMPILING
#pragma message "Compiling Drawing.cpp"
// or #warning "Compiling Drawing.cpp"?
#endif

#define XSIZE 800
#define YSIZE 300

#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wformat-security"


static int slider_position;
extern TFilter filter;
GtkWidget *app, /* *window, */ *cct_diagram;
GtkWidget *image;
double xyscale;
gchar str[256];
GtkWidget *Window1, *Area1;
GtkWidget *scale1;
GtkObject *scale_adj1;
static GdkPixmap *pixmap = NULL;

bool square = FALSE;
GdkColor TriColour;
// Combi boxes
GtkWidget *Ripple = NULL, *Bandwidth, *Type, *Type2, *Circuit, *Freq, *Atten, *SamplingFreq, *Order;
GtkWidget *LabelR, *LabelB, *LabelS;
// Buttons.
static GtkWidget *vbox1, *vbox2, *sp_button, *t_button, *f_button, *s_button /*, *z_button*/ ;
DrawMode Draw_mode;

class callback {
	public:
	static void clicked(GtkWidget *button, gpointer data);
	static gint quit(GtkWidget *Widget, GdkEvent *event, gpointer data);
};


/** 
Define a method  to be called if a button is clicked.
Read all the combo-boxes and correct the filter object in
case calculations have already been done.

Currently we do not check for valid entries.
*/
void callback::clicked(GtkWidget *button, gpointer data)
{
void DrawMap(GtkWidget *, gdouble, gdouble);
void Draw_poles(GtkWidget *widget, gdouble x, gdouble y);
void DrawStep(GtkWidget *widget, gdouble x, gdouble y);
void DrawBode(GtkWidget *widget, gdouble x, gdouble y);
double scale(TFilter);
	gchar *text;
	int w, h;
	int n;
	char *string = (char *)data;
	// OK button clicked.
	g_print("<callback::clicked>\n");
	g_print(string);
	text = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(Type));
	if(strncmp(text, "L", 1) == 0) filter.fclass = Lowpass;
	else if(strncmp(text, "H", 1) == 0) filter.fclass = Highpass;
	else if(strncmp(text, "B", 1) == 0) filter.fclass = Bandpass;

	text = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(Type2));
	if(strncmp(text, "Be", 2) == 0) filter.sclass = Bessel;
	else if(strncmp(text, "Bu", 2) == 0) filter.sclass = Butterworth;
	else if(strncmp(text, "C", 1) == 0) filter.sclass = Chebyshev;

	text = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(Circuit));
	if(strncmp(text, "S", 1) == 0) filter.cclass = SallKey;
	else if(strncmp(text, "R", 1) == 0) filter.cclass = Rauch;
	else if(strncmp(text, "Z", 1) == 0) filter.cclass = ZDomain;

	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Freq)->entry));
	if(strlen(text)) n = sscanf(text, "%lf", &filter.frequency);
	if((n == 0) || (filter.frequency <= 0.0)) {
		filter.frequency = 1.0;
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Freq)->entry), "?");
	}

	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Order)->entry));
	if(strlen(text)) sscanf(text, "%d", &filter.poles);
	if((n == 0) || (filter.poles < 2) || ((filter.poles < 3) && (filter.fclass != Bandpass))) {
		filter.poles = 3;
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Order)->entry), "?");
	}

	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Ripple)->entry));
	if(strlen(text)) sscanf(text, "%lf", &filter.ripple);
	if((n == 0) || (filter.ripple < 0.0)) {
		filter.ripple = 1.0;
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Ripple)->entry), "?");
	}

	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(Bandwidth)->entry));
	if(strlen(text)) sscanf(text, "%lf", &filter.bandwidth);
	if((n == 0) || (filter.bandwidth <= 2)) {
		filter.bandwidth = filter.frequency/2.0;
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(Bandwidth)->entry), "?");
	}

	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(SamplingFreq)->entry));
	if(strlen(text)) sscanf(text, "%lf", &filter.samplingfreq);

	filter.Calculate( );
	bode_calc(filter);
	filter.step_calc( );
	xyscale = scale(filter);
	gdk_window_get_size(Window1->window, &w, &h);

	if(strncmp(string, "Syn", 3) == 0) {
		switch(filter.cclass) {
			case Rauch: filter.gain = -1.0; filter.Synth_Rauch( );
			switch(filter.fclass) {
				case Lowpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/low_pass_MF.png"); break;
				case Bandpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/band_pass_MF.png"); break;
				case Highpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/high_pass_MF.png"); break;
			}
			gtk_widget_show(image);
			gtk_widget_show(cct_diagram);
			break;
			case SallKey: filter.gain = 1.0; filter.Synth_SallKey( ); 
			switch(filter.fclass) {
				case Lowpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/low_pass_SK.png"); break;
				case Bandpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/band_pass_SK.png"); break;
				case Highpass: gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/high_pass_SK.png"); break;
			}
			gtk_widget_show(image);
			gtk_widget_show(cct_diagram);
			break;
			case ZDomain: filter.gain = 1.0; filter.bilinear( ); 
			gtk_image_set_from_file ((GtkImage *)image, "./Circuit_diagrams/DirectForm1.png");
			gtk_widget_show(image);
			gtk_widget_show(cct_diagram);
			break;
			default: cout << "Unknown cclass!\n"; exit(-1);
		}
	}

	if(strncmp(string, "S-p", 2) == 0) {
		gtk_widget_hide(scale1);
		Draw_mode = Splane;
		Draw_poles(Area1, 250, 250);
	}
	if(strncmp(string, "T-d", 2) == 0) {
		gtk_adjustment_set_value (GTK_ADJUSTMENT(scale_adj1), 0.0);
		gtk_widget_show(scale1);
		Draw_mode = Step;
		DrawStep(Area1, 250, 250);
	}
	if(strncmp(string, "F-d", 2) == 0) {
		gtk_adjustment_set_value (GTK_ADJUSTMENT(scale_adj1), (double)FSTEPS/2.0);
		gtk_widget_show(scale1);
		Draw_mode = Bode;
		DrawBode(Area1, 250, 250);
	}

	g_print("</callback::clicked>\n");
}



/// Define a method  to be called if the close icon is clicked.
gint callback::quit(GtkWidget *Widget, GdkEvent *event, gpointer data)
{
	g_print("'Quit' callback\n");

	gtk_main_quit( );
	return FALSE;
}



/// I would like to make the application ignore the second window 'quit' event, but since I can't I'll quit the application.
gint pseudo_quit(GtkWidget *Widget, GdkEvent *event, gpointer data)
{
	g_print("Second window 'Quit' callback\n");
	gtk_main_quit( );
	return FALSE;
}



/// Draw the Bode diagram.
void DrawBode(GtkWidget *widget, gdouble x, gdouble y)
{
	extern double n_to_freq(int n, double f1, int nmax=FSTEPS, int decades=2);
	char str[80];
	gint i, w, h;
	gint xx, yy;
	gint xx1, yy1, xx2, yy2;
	gint width, height;
	double a, f;
	GdkFont* Font1;
	GdkColor Colour1, Colour2;
	GdkGC *gc1= gdk_gc_new(widget->window);
	GdkGC *gc2= gdk_gc_new(widget->window);

#if VERBOSE
	g_print("<DrawBode>\n");
#endif

	gtk_window_set_title ((GtkWindow *)Window1,  (gchar *)"Bode diagram");

// Let's colour 'gc1' blue and set the foreground colour to it.
	Colour1.red=32000;
	Colour1.green=32000;
	Colour1.blue=62000;
	gdk_gc_set_rgb_fg_color(gc1, &Colour1);

	Colour2.red=32000;
	Colour2.green=62000;
	Colour2.blue=32000;
	gdk_gc_set_rgb_fg_color(gc2, &Colour2);

//	g_print("draw brush\n");
  Font1 = gdk_font_load("*trebuchet*12*");
  if(Font1 == NULL) Font1 = gdk_font_load("*arial*");
  if(Font1 == NULL) Font1 = gdk_font_load("*sans*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*--14*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*");
  if(Font1 == NULL) Font1 = gdk_font_load("-misc-fixed-*");
  if(Font1 == NULL) Font1 = gdk_font_load("-bitstream-bitstream charter-*");
  if(Font1 == NULL) Font1 = gdk_font_load("*clean*");
  if(Font1 == NULL) g_print("Failed to find font\n");

	gdk_window_get_size(widget->window, &w, &h);
//  OR gdk_drawable_get_size(widget->window, &width, &height);

	GdkRectangle update_rect;

	update_rect.x = (gint)1;
	update_rect.y = (gint)1;
	update_rect.width = (gint)w - 2;
	update_rect.height = (gint)h - 2;

	gdk_draw_rectangle(widget->window, widget->style->white_gc, TRUE, 0, 0, w, h);
	gdk_draw_line(widget->window, gc1, 10, h/5, w-10, h/5);
	gdk_draw_line(widget->window, gc1, (w)/10, h, (w)/10, 1);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (w)/10, h/5, "0,0");

	yy = h/5 - 0.5*log10(0.708) * 3*h/5;
	gdk_draw_line(widget->window, gc2, w/10, yy, 9*w/10, yy);
	gdk_draw_string(widget->window, Font1, gc2, (w)/10, yy, "-3dB");
	yy = h/5 - 0.5*log10(0.708*0.708) * 3*h/5;
	gdk_draw_line(widget->window, gc2, w/10, yy, 9*w/10, yy);
	gdk_draw_string(widget->window, Font1, gc2, (w)/10, yy, "-6dB");
	yy = h/5 - 0.5*log10(0.708*0.708*0.708) * 3*h/5;
	gdk_draw_line(widget->window, gc2, w/10, yy, 9*w/10, yy);
	gdk_draw_string(widget->window, Font1, gc2, (w)/10, yy, "-9dB");
	if(filter.fclass == Bandpass) gdk_draw_string(widget->window, Font1, widget->style->black_gc, (w)/10, yy/10, "Gain normalised to 1.0");

/// Draw the frequency response curve.
	for(i=1; i<FSTEPS; i++) {
		xx1 = w/10 + (i-1) * 8*w/10 / FSTEPS;
		yy1 = h/5 - 0.5*log10(filter.freq_resp[i-1]) * 3*h/5;
		xx2 = w/10 + i * 8*w/10 / FSTEPS;
		yy2 = h/5 - 0.5*log10(filter.freq_resp[i]) * 3*h/5;
		gdk_draw_line(widget->window, gc1, xx1, yy1, xx2, yy2);
	}

/// Draw line at measurement frequency
	f = n_to_freq(slider_position, filter.fmax);
	xx1 = w/10 + (slider_position-1) * 8*w/10 / FSTEPS;
	gdk_draw_line(widget->window, gc1, xx1, h, xx1, 1);
	sprintf(str, "f = %0.2f Hz", f);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (7*w)/10, h/10, str);
//	a = log10(filter.freq_resp[slider_position-1]);
	a = filter.freq_resp[slider_position-1];
	sprintf(str, "Gain = %0.2f ", a);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (7*w)/10, h/8, str);

#if VERBOSE
	g_print("</DrawBode>\n");
#endif
}



/// Draw the s-plane diagram.
void Draw_poles(GtkWidget *widget, gdouble x, gdouble y)
{
	double scale(TFilter);

	char str[80];
	gint i, w, h;
	gint xx, yy;
	gint width, height;
	GdkFont* Font1;
	GdkColor Colour1;
	GdkGC *gc1= gdk_gc_new(widget->window);

#if VERBOSE
	g_print("<Draw_poles>\n");
#endif

	gtk_window_set_title ((GtkWindow *)Window1,  (gchar *)"'s' plane");

// Let's colour 'gc1' blue and set the foreground colour to it.
	Colour1.red=32000;
	Colour1.green=32000;
	Colour1.blue=62000;
	gdk_gc_set_rgb_fg_color(gc1, &Colour1);

//	g_print("draw brush\n");
  Font1 = gdk_font_load("*trebuchet*12*");
  if(Font1 == NULL) Font1 = gdk_font_load("*arial*");
  if(Font1 == NULL) Font1 = gdk_font_load("*sans*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*--14*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*");
  if(Font1 == NULL) Font1 = gdk_font_load("-misc-fixed-*");
  if(Font1 == NULL) Font1 = gdk_font_load("-bitstream-bitstream charter-*");
  if(Font1 == NULL) Font1 = gdk_font_load("*clean*");
  if(Font1 == NULL) g_print("Failed to find font\n");

	gdk_window_get_size(widget->window, &w, &h);
//  OR gdk_drawable_get_size(widget->window, &width, &height);

	GdkRectangle update_rect;

	update_rect.x = (gint)1;
	update_rect.y = (gint)1;
	update_rect.width = (gint)w - 2;
	update_rect.height = (gint)h - 2;

	gdk_draw_rectangle(widget->window, widget->style->white_gc, TRUE, 0, 0, w, h);
	gdk_draw_line(widget->window, gc1, 10, h/2, w-10, h/2);
	gdk_draw_line(widget->window, gc1, (4*w)/5, h, (4*w)/5, 1);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (4*w)/5+5, h/2-5, "0,0");

	xyscale = scale(filter);
	 
	for(i=0; i<filter.poles; i++) {
		xx = filter.st[i].pole.real() * ((3 * w) / 5) / xyscale + (4 * w) / 5;
		yy = filter.st[i].pole.imag() * (3 * h / 5) / xyscale;
#if SHOW_XY
		cout << "x, y = " << xx << ", " << yy << "\n";
#endif
		gdk_draw_line(widget->window, widget->style->black_gc, xx-10, h/2 + yy, xx+10, h/2 + yy);
		gdk_draw_line(widget->window, widget->style->black_gc, xx, h/2 + yy - 10, xx, h/2 + yy + 10);
		sprintf(str, "%1.3lf, %1.2lf", filter.st[i].pole.real(), filter.st[i].pole.imag());
		gdk_draw_string(widget->window, Font1, gc1, xx - 80, h/2 + yy - 15, str);
	}
	if(filter.type != '0') for(i=0; i<filter.poles; i+=2) {
		if(i == filter.poles - 1) sprintf(str, "Tau = %1.3lf ", filter.st[i/2].T);
		else sprintf(str, "T = %1.3lf, q = %1.3lf", filter.st[i/2].T, filter.st[i/2].q);
		gdk_draw_string(widget->window, Font1, widget->style->black_gc, w / 10, 20 + i * 15, str);
	}
	if(filter.zeroes) {
		gdk_draw_arc(widget->window, widget->style->black_gc, false, (4*w)/5 - 10, h/2 - 10, 20, 20, 0, 360*64);
		sprintf(str, "* %1d", filter.zeroes);
		gdk_draw_string(widget->window, Font1, gc1, (4*w)/5 - 20, h/2 + 20, str);
	}
#if VERBOSE
	g_print("</Draw_poles>\n");
#endif
}



/// Draw the filter's step response.
void DrawStep(GtkWidget *widget, gdouble x, gdouble y)
{
	char str[80];
	gint i, w, h;
	gint xx1, yy1, xx2, yy2;
	gint width, height;
	double t, v;
	GdkFont* Font1;
	GdkColor Colour1;
	GdkGC *gc1= gdk_gc_new(widget->window);

#if VERBOSE
	g_print("<DrawStep>\n");
#endif

	gtk_window_set_title ((GtkWindow *)Window1,  (gchar *)"Time domain");

/// Let's colour 'gc1' blue and set the foreground colour to it.
	Colour1.red=32000;
	Colour1.green=32000;
	Colour1.blue=62000;
	gdk_gc_set_rgb_fg_color(gc1, &Colour1);

/// Select a suitable font.
//	g_print("draw brush\n");
  Font1 = gdk_font_load("*trebuchet*12*");
  if(Font1 == NULL) Font1 = gdk_font_load("*arial*");
  if(Font1 == NULL) Font1 = gdk_font_load("*sans*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*--14*");
  if(Font1 == NULL) Font1 = gdk_font_load("*helvetica*");
  if(Font1 == NULL) Font1 = gdk_font_load("-misc-fixed-*");
  if(Font1 == NULL) Font1 = gdk_font_load("-bitstream-bitstream charter-*");
  if(Font1 == NULL) Font1 = gdk_font_load("*clean*");
  if(Font1 == NULL) g_print("Failed to find font\n");

/// Check the size of the window then define an update rectangle.
	gdk_window_get_size(widget->window, &w, &h);
//  OR gdk_drawable_get_size(widget->window, &width, &height);

	GdkRectangle update_rect;
	update_rect.x = (gint)1;
	update_rect.y = (gint)1;
	update_rect.width = (gint)w - 2;
	update_rect.height = (gint)h - 2;

	filter.step_calc( );

/// Draw suitable axes.
	gdk_draw_rectangle(widget->window, widget->style->white_gc, TRUE, 0, 0, w, h);
	switch(filter.fclass) {
		case Lowpass: gdk_draw_line(widget->window, gc1, 10, 4*h/5, w-10, 4*h/5); break;
		case Bandpass: gdk_draw_line(widget->window, gc1, 10, h/2, w-10, h/2); break;
		case Highpass: gdk_draw_line(widget->window, gc1, 10, 3*h/5, w-10, 3*h/5); break;
	}
	gdk_draw_line(widget->window, gc1, w/10, h, w/10, 1);

/// Draw the step response curve.
	for(i=1; i<TSTEPS; i++) {
		xx1 = w/10 + (i-1) * 8*w/10 / TSTEPS;
		xx2 = w/10 + i * 8*w/10 / TSTEPS;
		switch(filter.fclass) {
			case Lowpass:
			yy1 = 4*h/5 - filter.step_resp[i-1] * 3*h/5;
			yy2 = 4*h/5 - filter.step_resp[i] * 3*h/5;
			break;
			case Bandpass:
			yy1 = h/2 - filter.step_resp[i-1] * 3*h/10;
			yy2 = h/2 - filter.step_resp[i] * 3*h/10;
			break;
			case Highpass: 
			yy1 = 3*h/5 - filter.step_resp[i-1] * 5*h/10;
			yy2 = 3*h/5 - filter.step_resp[i] * 5*h/10;
			default: break;
		}
		gdk_draw_line(widget->window, gc1, xx1, yy1, xx2, yy2);
	}

/// Draw line at measurement time
	t = filter.tmax * slider_position / TSTEPS;
	xx1 = w/10 + (slider_position-1) * 8*w/10 / FSTEPS;
	gdk_draw_line(widget->window, gc1, xx1, h, xx1, 1);
	sprintf(str, "t = %0.2f ", t);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (7*w)/10, h/8, str);
	v = filter.step_resp[slider_position-1];
	sprintf(str, "v = %0.2f at ", v);
	gdk_draw_string(widget->window, Font1, widget->style->black_gc, (7*w)/10, h/10, str);

/// List the stage Ts and qs etc.
	if(filter.type != '0') for(i=0; i<filter.poles; i+=2) {
		if(i == filter.poles - 1) sprintf(str, "Tau = %1.3lf ", filter.st[i/2].T);
		else sprintf(str, "T = %1.3lf, q = %1.3lf", filter.st[i/2].T, filter.st[i/2].q);
		gdk_draw_string(widget->window, Font1, widget->style->black_gc, w / 10, 20 + i * 15, str);
	}
#if VERBOSE
	g_print("</DrawStep>\n");
#endif
}



/** Draw a rectangle on the screen, and add the axes, poles and
the Ts and qs. */
void
DrawMap(GtkWidget *widget, gdouble x, gdouble y)
{
#if VERBOSE
	g_print("<DrawMap>\n");
#endif

	switch(Draw_mode) {
		case Splane: Draw_poles(widget, 250, 250);
		break;
		case Step: DrawStep(widget, 250, 250);
		break;
		case Bode: DrawBode(widget, 250, 250);
		break;
		default: cout << "\n\n***UNKNOWN DRAW MODE!***\n\n";
		break;
	}
#if VERBOSE
	g_print("</DrawMap>\n");
#endif
} 



/** Define a function to be called if the window is re-exposed. */
static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event)
{
#if VERBOSE
	g_print("<expose_event>\n");
#endif

	DrawMap(widget, 0, 0);

#if VERBOSE
	g_print("</expose_event>\n");
#endif

	return(TRUE);
}



/** If a button is pressed, do what? */
static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event )
{
	gint w, h;

#if VERBOSE
	g_print("<button_press_event>\n");
#endif

	gdk_window_get_size(widget->window, &w, &h);
	g_print("button press callback\n");

	gdk_draw_rectangle(widget->window, widget->style->white_gc, TRUE, 0, 0, w, h);
	gdk_draw_rectangle(widget->window, widget->style->black_gc, FALSE, (w/2)-4, (h/2)-4, 8, 8);
	if (event->button == 1) DrawMap (widget, event->x, event->y);

#if VERBOSE
	g_print("</button_press_event>\n");
#endif

	return TRUE;
}



/** Redraw the diagram if pointer moves. */
static gboolean
motion_notify_event( GtkWidget *widget, GdkEventMotion *event )
{
	int x, y;
	GdkModifierType state;

#if VERBOSE
	g_print("<motion_notify_event>\n");
#endif

	if (event->is_hint) gdk_window_get_pointer (event->window, &x, &y, &state);
	else {
		x = (gint)event->x;
		y = (gint)event->y;
		state = (GdkModifierType)event->state;
	}
	    
//  if (state & GDK_BUTTON1_MASK) DrawMap (window, x, y);
	DrawMap(widget, x, y);

#if VERBOSE
	g_print("</motion_notify_event>\n");
#endif
	  
	return TRUE;
} 



/*!
'configure_event( )' redraws the window during resizing.
*/
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event )
{
	gint x, y;
#if VERBOSE
	g_print("<configure_event>\n");
#endif

	if(pixmap) g_object_unref(pixmap);

	pixmap = gdk_pixmap_new(widget->window,
	widget->allocation.width, widget->allocation.height, -1);
	gdk_draw_rectangle (widget->window, widget->style->white_gc,
	TRUE, 0, 0, x = widget->allocation.width, y = widget->allocation.height);
	DrawMap(widget, x, y);
#if VERBOSE
	g_print("</configure_event>\n");
#endif

	return TRUE;
} 



bool Check_band(void)
{
const gchar *txt = "Impossible request";
// Buttons are:- sp_button, t_button, f_button, s_button
	if(filter.fclass == Bandpass && filter.poles % 2) {
		gtk_widget_set_sensitive(sp_button, false);
		gtk_widget_set_sensitive(t_button, false);
		gtk_widget_set_sensitive(f_button, false);
		gtk_widget_set_sensitive(s_button, false);
/*		GtkWidget* msg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, txt);
		gtk_widget_show(GTK_WIDGET(msg)); */
	} else {
		gtk_widget_set_sensitive(sp_button, true);
		gtk_widget_set_sensitive(t_button, true);
		gtk_widget_set_sensitive(f_button, true);
		gtk_widget_set_sensitive(s_button, true);
	}
}



/*!
'Type_event' call-back function to process changed data in filter class combo-box.
*/
void Type_event(GtkComboBox *widget, gpointer data)
{
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	/// Get the text box contents.
	entry_text = gtk_combo_box_get_active_text(widget);
	if(strncmp(entry_text, "L", 1) == 0) filter.fclass = Lowpass;
	else if(strncmp(entry_text, "H", 1) == 0) filter.fclass = Highpass;
	else if(strncmp(entry_text, "B", 1) == 0) {
		filter.fclass = Bandpass;
		gtk_widget_show(GTK_WIDGET(LabelB));
		gtk_widget_show(GTK_WIDGET(Bandwidth));
	} else {
		gtk_widget_hide(GTK_WIDGET(LabelB));
		gtk_widget_hide(GTK_WIDGET(Bandwidth));
	}
//	Check_band( );
	printf("Class: <%s>\n", entry_text);
}



/*!
'Type2_event' call-back function to process changed data in filter class combo-box.
*/
void Type2_event(GtkComboBox *widget, gpointer data)
{
	gchar *entry_text;
	gtk_widget_hide(image);

	// Get the text box contents.
	entry_text = gtk_combo_box_get_active_text(widget);
	if(strncmp(entry_text, "Be", 2) == 0) filter.sclass = Bessel;
	else if(strncmp(entry_text, "Bu", 2) == 0) filter.sclass = Butterworth;
	else if(strncmp(entry_text, "C", 1) == 0) {
		filter.sclass = Chebyshev;
		gtk_widget_show(GTK_WIDGET(LabelR));
		gtk_widget_show(GTK_WIDGET(Ripple));
	} else {
		gtk_widget_hide(GTK_WIDGET(LabelR));
		gtk_widget_hide(GTK_WIDGET(Ripple));
	}

//	Check_band( );
	printf("Type: <%s>\n", entry_text);
}




/*!
'Circuit_event' call-back function to process changed data in filter class combo-box.
*/
void Circuit_event(GtkComboBox *widget, gpointer data)
{
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	// Get the text box contents.
	entry_text = gtk_combo_box_get_active_text(widget);
	if(strncmp(entry_text, "S", 1) == 0) filter.cclass = SallKey;
	else if(strncmp(entry_text, "R", 1) == 0) filter.cclass = Rauch;
	else if(strncmp(entry_text, "D", 1) == 0) {
		filter.cclass = ZDomain;
		gtk_widget_show(GTK_WIDGET(LabelS));
		gtk_widget_show(GTK_WIDGET(SamplingFreq));
	} else {
		gtk_widget_hide(GTK_WIDGET(LabelS));
		gtk_widget_hide(GTK_WIDGET(SamplingFreq));
	}
//  if(strncmp(entry_text, "B", 1) == 0) filter.cclass = ????;
//	Check_band( );
	printf("Type: <%s>\n", entry_text);
}



/*!
'Freq_callback' call-back function to process changed data in frequency combo-box.
*/
void Freq_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	double T1;
// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &filter.frequency);
//	Check_band( );
	printf("Entry contents 1: <%s>\n", entry_text);

// Clear box by writing empty string? May cause a problem when you type text!
/*	if((n == 0) || (filter.frequency <= 0.0)) {
		gtk_entry_set_text (GTK_ENTRY(widget), "1.0");
		filter.frequency = 1.0;
	}
*/
}



/*!
'Atten_callback' call-back function to process changed data in attenuation combo-box.
*/
void Atten_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	double a;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	double T1;
// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &a);

// Clear box by writing empty string? May cause a problem when you type text!
//	if((n == 0) || (a > 0.0)) { gtk_entry_set_text (GTK_ENTRY(widget), "-3.0"); a = -3.0; }

/** dB = 20 * log10(ratio)
e.g. if ratio = 0.707, dB = 3.01
Thus, if we have the value in dB:
ratio = pow10(dB / 20)
*/
	filter.a0 = pow10(-abs(a) / 20.0);

//	Check_band( );
	printf("Entry contents 1a: <%s>\n", entry_text);
}



/*!
'SFreq_callback' call-back function to process changed data in frequency combo-box.
*/
void SFreq_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	double T1;
	// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &filter.frequency);
/*	Check_band( ); */
	printf("Entry contents 1a: <%s>\n", entry_text);
	// Clear box by writing empty string? May cause a problem when you type text!
/*	if(n == 0) {
		gtk_entry_set_text (GTK_ENTRY(widget), "1.0");
		filter.frequency = 1.0;
	}
*/
}



/*!
'q_callback' call-back function to process changed data in filter 'q' combo-box.
*/
void q_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
//	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &filter.q);
//	Check_band( );
	printf("Entry contents 2: <%s>\n", entry_text);
	// Clear box by writing empty string? May cause a problem when you type text!
//	if(n == 0) gtk_entry_set_text (GTK_ENTRY(widget), "?");
}



/*!
'order_callback' call-back function to process changed data in filter 'order' combo-box.
*/
void order_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%d", &filter.poles);
//	Check_band( );
	printf("Entry contents 3: <%s>\n", entry_text);
	// Clear box by writing empty string? May cause a problem when you type text!
//	if(n == 0) gtk_entry_set_text (GTK_ENTRY(widget), "?");
}



/*!
'ripple_callback' call-back function to process changed data in filter 'ripple' combo-box.
*/
void ripple_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &filter.ripple);
//	Check_band( );
	printf("Entry contents 4: <%s>\n", entry_text);
	// Clear box by writing empty string? May cause a problem when you type text!
//	if(n == 0) gtk_entry_set_text(GTK_ENTRY(widget), "?");
}



/*!
'bandwidth_callback' call-back function to process changed data in filter 'ripple' combo-box.
*/
void bandwidth_callback(GtkWidget *widget, GtkWidget *textbox)
{
	int n;
	gchar *entry_text;
	gtk_widget_hide(cct_diagram);

	// Get the text box contents.
	entry_text = (gchar *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(textbox)->entry));
	if(strlen(entry_text)) n = sscanf(entry_text, "%lf", &filter.bandwidth);
//	Check_band( );
	printf("Entry contents 5: <%s>\n", entry_text);
	// Clear box by writing empty string? May cause a problem when you type text!
//	if(n == 0) gtk_entry_set_text (GTK_ENTRY(widget), "?");
}




/** 'time_event' reads slider value at regular intervals. This avoids large
numbers of redrawings as the slider is moved.
*/
gboolean time_event(GtkWidget *widget)
{
	static int last_value = -1;

	if (widget->window == NULL) return FALSE;

	GDateTime *now = g_date_time_new_now_local(); 
	gchar *my_time = g_date_time_format(now, "%H:%M:%S");

	slider_position = gtk_adjustment_get_value(GTK_ADJUSTMENT(scale_adj1));
	if(slider_position != last_value) {
		sprintf(str, "Scale position = %d\n", slider_position);
		last_value = slider_position;
#if VERBOSE
		g_print(str);
#endif
		gtk_widget_queue_draw (widget);
	}

	g_free(my_time);
	g_date_time_unref(now);

	return TRUE;
}




int main(int argc, char *argv[ ])
{
	std::cout << "'rbfilter' version " << VERSION << "\n";
	std::cout << "Copyright (c) Roger Burghall 2013..2016\n";

	ofstream logfile;
	logfile.open("./rbfilter.log", ios::trunc);
	logfile << "rbfilter log file.\n******************\n";
	logfile.close( );

	GtkWidget *hbox, *vbox_draw, *vbox_main;
	GList *glist, *glist1;

	GtkWidget *vbox_cct;

//* Initialise Gnome; similar to gtk_init(). */
	gnome_init("Filter-designer", "3.01", argc, argv);
/// First window, for controls.
	app = gnome_app_new("drawing-example", "Control");
	assert(app != NULL);
/// Draw a second window  for graphs etc.
	Window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	assert(Window1 != NULL);
	/// Centre the graphics window.
	gtk_window_set_position ((GtkWindow *)Window1, GTK_WIN_POS_CENTER);
	gtk_widget_set_size_request(GTK_WIDGET(Window1), 600, 650);
	gtk_window_set_title ((GtkWindow *)Window1,  (gchar *)"Graphics");

	vbox_draw = gtk_vbox_new(FALSE, 5);

	scale_adj1 = gtk_adjustment_new (FSTEPS/2.0, 0.0, (double)FSTEPS, 10.0, 10.0, 10.0);
	scale1 = gtk_hscale_new (GTK_ADJUSTMENT(scale_adj1));
	gtk_scale_set_draw_value ((GtkScale*)scale1, FALSE);

/// Put a vbox in the window and the slider in the vbox.
	gtk_container_add(GTK_CONTAINER(Window1), vbox_draw);
	gtk_box_pack_start(GTK_BOX(vbox_draw), scale1, FALSE, FALSE, 0);

	gtk_window_set_default_size (GTK_WINDOW(Window1), 400, 300);

/// Create a drawing area which must receive extra space when window is enlarged.
	Area1 = gtk_drawing_area_new( );
	gtk_widget_set_size_request(GTK_WIDGET(Area1), XSIZE, YSIZE);
	gtk_box_pack_start(GTK_BOX(vbox_draw), Area1, TRUE, TRUE, 0);

	gtk_widget_show(vbox_draw);
	gtk_widget_show(Area1);
	gtk_widget_hide(scale1);

/// Draw a third window, for showing the circuit diagrams
	cct_diagram = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	assert(cct_diagram != NULL);
	gtk_window_set_position ((GtkWindow *)cct_diagram, GTK_WIN_POS_NONE);
	gtk_widget_set_size_request(GTK_WIDGET(cct_diagram), 650, 365);
	gtk_window_set_title ((GtkWindow *)cct_diagram,  (gchar *)"Realisation");
	GtkWidget *cct_box = gtk_hbox_new(false, 0);
	image = gtk_image_new( );
	gtk_container_add(GTK_CONTAINER(cct_diagram), cct_box);
	gtk_box_pack_start(GTK_BOX(cct_box), image, false, false, 3);
/// Remove close icon from the realisation window.
	gtk_window_set_deletable((GtkWindow *)cct_diagram, false);
	gtk_widget_show(image);
	gtk_widget_show(cct_box);
	gtk_widget_hide(cct_diagram);

	Draw_mode = Splane;

/// Create a 'vbox' and place it in the application.
	vbox_main = gtk_vbox_new(FALSE, 5);
	gnome_app_set_contents(GNOME_APP(app), vbox_main);
	gtk_widget_show(vbox_main);

/// Position the control window to the left of the screen.
	/// Check size of screen.
	gint scr_w = gdk_screen_width( );
	gint scr_h = gdk_screen_height( );
	/// Place the window.
	gtk_window_move ((GtkWindow *)app, scr_w/10, scr_h/5);

/// Create an 'hbox' and put it in the 'vbox'
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add (GTK_CONTAINER (vbox_main), hbox);
	gtk_widget_show(hbox);

/// Create button 'vbox'es and place them in the 'hbox'.
	vbox1 = gtk_vbox_new(FALSE, 5);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	gtk_widget_show(vbox1);
	gtk_widget_show(vbox2);

/// Put data entry boxes in 'vbox1':-
/// Combobox permitting only preset entries for filter class
	GtkWidget *Label = gtk_label_new ("Class");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(Label));
	Type = /* GTK_COMBO_BOX */ (gtk_combo_box_new_text( ));
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type), "Low-pass");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type), "High-pass");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type), "Band-pass");
	gtk_combo_box_set_active(GTK_COMBO_BOX(Type), 0);
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Type), TRUE, TRUE, 5);
	g_signal_connect(GTK_OBJECT(Type), "changed", GTK_SIGNAL_FUNC(Type_event), Type);
	gtk_widget_show(GTK_WIDGET(Type));

	Type2 = /* GTK_COMBO_BOX */ (gtk_combo_box_new_text( ));
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type2), "Bessel");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type2), "Butterworth");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Type2), "Chebyshev");
	gtk_combo_box_set_active(GTK_COMBO_BOX(Type2), 0);
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Type2), TRUE, TRUE, 5);
	g_signal_connect(GTK_OBJECT(Type2), "changed", GTK_SIGNAL_FUNC(Type2_event), Type2);
	gtk_widget_show(GTK_WIDGET(Type2));

/// Combobox permitting only preset entries for filter class
	Label = gtk_label_new ("Circuit");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(Label));
	Circuit = /* GTK_COMBO_BOX */ (gtk_combo_box_new_text( ));
	gtk_combo_box_append_text(GTK_COMBO_BOX(Circuit), "Sallen and Key");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Circuit), "Rauch");
	gtk_combo_box_append_text(GTK_COMBO_BOX(Circuit), "Discrete");
	gtk_combo_box_set_active(GTK_COMBO_BOX(Circuit), 0);
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Circuit), TRUE, TRUE, 5);
	g_signal_connect(GTK_OBJECT(Circuit), "changed", GTK_SIGNAL_FUNC(Circuit_event), Circuit);
	gtk_widget_show(GTK_WIDGET(Circuit));

/// Combobox permitting non-standard entries for cut-off frequency
	Label = gtk_label_new ("Freq");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(Label));
	Freq = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Freq), TRUE, FALSE, 0);
	glist = NULL;
	glist = g_list_append(glist, (gpointer)"1.0");
	glist = g_list_append(glist, (gpointer)"10.0");
	glist = g_list_append(glist, (gpointer)"100.0");
	gtk_combo_set_popdown_strings(GTK_COMBO(Freq), glist);
	g_signal_connect(GTK_COMBO(Freq)->entry, "changed", G_CALLBACK(Freq_callback), Freq);
	gtk_widget_show(GTK_WIDGET(Freq));

/// Combobox permitting non-standard entries for cut-off attenuation
	Label = gtk_label_new ("Atten (dB)");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(Label));
	Atten = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Atten), TRUE, FALSE, 0);
	glist = NULL;
	glist = g_list_append(glist, (gpointer)"-3.0");
	glist = g_list_append(glist, (gpointer)"-6.0");
	glist = g_list_append(glist, (gpointer)"-9.0");
	gtk_combo_set_popdown_strings(GTK_COMBO(Atten), glist);
	g_signal_connect(GTK_COMBO(Atten)->entry, "changed", G_CALLBACK(Atten_callback), Atten);
	gtk_widget_show(GTK_WIDGET(Atten));

/// Combobox permitting non-standard entries for sampling frequency
	LabelS = gtk_label_new ("Sampling Freq");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelS), TRUE, FALSE, 0);
//	gtk_widget_show(GTK_WIDGET(Label));
	SamplingFreq = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(SamplingFreq), TRUE, FALSE, 0);
	glist = NULL;
	glist = g_list_append(glist, (gpointer)"100.0");
	glist = g_list_append(glist, (gpointer)"1000.0");
	glist = g_list_append(glist, (gpointer)"100000.0");
	gtk_combo_set_popdown_strings(GTK_COMBO(SamplingFreq), glist);
	g_signal_connect(GTK_COMBO(Freq)->entry, "changed", G_CALLBACK(SFreq_callback), SamplingFreq);
//	gtk_widget_show(GTK_WIDGET(SamplingFreq));

/// Combobox permitting non-standard entries for filter order
	Label = gtk_label_new ("Prototype order");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(Label));
	Order = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Order), TRUE, FALSE, 0);
	glist = NULL;
	glist = g_list_append(glist, (gpointer)"3");
	glist = g_list_append(glist, (gpointer)"4");
	glist = g_list_append(glist, (gpointer)"5");
	gtk_combo_set_popdown_strings(GTK_COMBO(Order), glist);
	g_signal_connect(GTK_COMBO(Order)->entry, "changed", G_CALLBACK(order_callback), Order);
	gtk_widget_show(GTK_WIDGET(Order));

/// Combobox permitting non-standard entries for filter ripple (Chebyshev only)
	LabelR = gtk_label_new ("ripple");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelR), TRUE, FALSE, 0);
	Ripple = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Ripple), TRUE, FALSE, 0);
	glist1 = NULL;
	glist1 = g_list_append(glist1, (gpointer)"0.5");
	glist1 = g_list_append(glist1, (gpointer)"1.0");
	glist1 = g_list_append(glist1, (gpointer)"2.0");
	gtk_combo_set_popdown_strings(GTK_COMBO(Ripple), glist1);
	g_signal_connect(GTK_COMBO(Ripple)->entry, "changed", G_CALLBACK(ripple_callback), Ripple);

/// Combobox permitting non-standard entries for filter bandwidth (Bandpass only)
	LabelB = gtk_label_new ("bandwidth");
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelB), TRUE, FALSE, 0);
	Bandwidth = gtk_combo_new( );
	gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Bandwidth), TRUE, FALSE, 0);
	glist1 = NULL;
	glist1 = g_list_append(glist1, (gpointer)"0.25");
	glist1 = g_list_append(glist1, (gpointer)"0.125");
	glist1 = g_list_append(glist1, (gpointer)"0.0625");
	gtk_combo_set_popdown_strings(GTK_COMBO(Bandwidth), glist1);
	g_signal_connect(GTK_COMBO(Bandwidth)->entry, "changed", G_CALLBACK(bandwidth_callback), Bandwidth);

/// Create "S-p", "T-d" and "F-d" buttons and place them in 'vbox2'.
	sp_button = gtk_button_new_with_label ("s-plane");
	t_button = gtk_button_new_with_label ("Time");
	f_button = gtk_button_new_with_label ("Freq");
	s_button = gtk_button_new_with_label ("Synth");
//	z_button = gtk_button_new_with_label ("Bilinear");
	gtk_box_pack_start (GTK_BOX (vbox2), sp_button, TRUE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox2), t_button, TRUE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox2), f_button, TRUE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vbox2), s_button, TRUE, FALSE, 20);
//	gtk_box_pack_start (GTK_BOX (vbox2), z_button, TRUE, FALSE, 20);
	gtk_widget_show(sp_button);
	gtk_widget_show(t_button);
	gtk_widget_show(f_button);
	gtk_widget_show(s_button);
//	gtk_widget_show(z_button);

/// Connect button signals
	gchar *Event2 = (gchar *)"clicked";
	gchar *Ctrl1 = (gchar *)"S-p\n";
	gchar *Ctrl2 = (gchar *)"T-d\n";
	gchar *Ctrl3 = (gchar *)"F-d\n";
	gchar *Ctrl4 = (gchar *)"Syn\n";
	gchar *Ctrl5 = (gchar *)"Z-t\n";
	gtk_signal_connect(GTK_OBJECT (sp_button), Event2, GTK_SIGNAL_FUNC (callback::clicked), Ctrl1);
	gtk_signal_connect(GTK_OBJECT (t_button), Event2, GTK_SIGNAL_FUNC (callback::clicked), Ctrl2);
	gtk_signal_connect(GTK_OBJECT (f_button), Event2, GTK_SIGNAL_FUNC (callback::clicked), Ctrl3);
	gtk_signal_connect(GTK_OBJECT (s_button), Event2, GTK_SIGNAL_FUNC (callback::clicked), Ctrl4);
//	gtk_signal_connect(GTK_OBJECT (z_button), Event2, GTK_SIGNAL_FUNC (callback::clicked), Ctrl5);

//* Bind "destroy" event to gtk_main_quit. */
	gchar *Event1 = "destroy";
	gtk_signal_connect(GTK_OBJECT (app), Event1,
	 GTK_SIGNAL_FUNC (callback::quit), NULL);
	gtk_signal_connect(GTK_OBJECT (Window1), Event1,
	 GTK_SIGNAL_FUNC (pseudo_quit), NULL);

	gtk_signal_connect(GTK_OBJECT(Area1), "expose_event", GTK_SIGNAL_FUNC(expose_event), NULL);
	gtk_signal_connect (GTK_OBJECT(Area1),"configure_event",
		      (GtkSignalFunc) configure_event, NULL);
	gtk_signal_connect (GTK_OBJECT (Area1), "motion_notify_event",
		      (GtkSignalFunc) motion_notify_event, NULL);
	gtk_signal_connect (GTK_OBJECT (Area1), "button_press_event",
		      GTK_SIGNAL_FUNC(button_press_event), NULL);

	g_timeout_add(500, (GSourceFunc) time_event, (gpointer) Window1);
	time_event(Window1);

	gtk_widget_set_events (Window1, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK); 
	gtk_drawing_area_size((GtkDrawingArea*)Area1, XSIZE, YSIZE);

	gtk_widget_show(app);
	gtk_widget_show(Area1);
	gtk_widget_show(Area1);
	gtk_widget_show(Window1);

	gtk_main( );

	return 0;
}




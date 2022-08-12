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
the filter to the z-domain (i.e. to adapt the design for discrete implementation).
These functions were already implemented in the BBC Model B version of the suite.

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

Version 1.01 August 2018 RB
Altered logic to hide unwanted combo-boxes when filter class, circuit type or response 
is changed.

Version 2.01 March 2022 RB
Conversion to GTK3 and Cairo graphics.

Version 2.1 March 31st 2022 RB
Candidate for actual use (Beta?).

Version 2.11 April 27th RB
Corrected Chebyshev calculation
Removed "Atten" input.

*/

/*! \file rbfilter.cpp
rbfilter.cpp contains code to display windows to provide
a GUI for controlling the calculation of active filters
and in order to display results.

If porting to another OS this file is expected to need considerable modification.
It is intended that operating system dependant code should be kept in this file only.
*/


#include <gtk/gtk.h>
#include <math.h>
#include <cairo.h>
#include <complex.h>
#include <assert.h>
#include <iostream>
#include <X11/Xlib.h>
#include <string.h>
#include "Drawing.h"
#include "Calcs.h"


#define VERSION "2.2"

#if SHOW_FILE_COMPILING
#pragma message "Compiling Drawing.cpp"
// or #warning "Compiling Drawing.cpp"?
#endif

#define XSIZE 640
#define YSIZE 480
#define ZOOM_X  75.0
#define ZOOM_Y  75.0

#define HEIGHT  400
#define WIDTH   300

#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wformat-security"

GtkWidget *window1;         /// Window for controls.
GtkWidget *window2;         /// Window for graphs.


static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);


cairo_t *cr;

static gdouble slider_position;
extern TFilter filter;

GtkWidget *app, /* *window, */ *cct_diagram;
GtkWidget *image;
double xyscale;
gchar str[256];
GtkWidget *Window1, *Area1;
GtkWidget *scale1;
GtkAdjustment *scale_adj1;
static GdkPixbuf *pixmap = NULL;

bool square = FALSE;
GdkColor TriColour;
// Combi boxes
GtkWidget *Ripple = NULL, *Bandwidth, *Type, *Type2, *Circuit, *Freq, *Atten, *SamplingFreq, *Order;
GtkWidget *LabelR, *LabelB, *LabelS;
// Buttons.
static GtkWidget *vbox1, *vbox2, *sp_button, *t_button, *f_button, *s_button /*, *z_button*/ ;
DrawMode Draw_mode;
/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;



class callback {
    public:
    static void clicked(GtkWidget *button, gpointer data);
    static gint quit(GtkWidget *Widget, GdkEvent *event, gpointer data);
};



char *Dstring = "";
/** 
Define a method  to be called if a button is clicked.
Read all the combo-boxes and correct the filter object in
case calculations have already been done.

Currently we do not check for valid entries.
*/
void callback::clicked(GtkWidget *button, gpointer data)
{
void DrawMap(cairo_t *cr, gdouble, gdouble);
void Draw_poles(cairo_t *cr, gdouble x, gdouble y);
void DrawStep(cairo_t *cr, gdouble x, gdouble y);
void DrawBode(GtkWidget *area, cairo_t *cr, gdouble x, gdouble y);
// void on_draw(void);
// static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);

    double scale(TFilter);
    const gchar *text;
    int i, w, h;
    int n;
//    cairo_t cr1;
    Dstring = (char *)data;
    // OK button clicked.
#if VERBOSE
    g_print("<callback::clicked>\n");
    g_print(Dstring);
#endif
    /// Read 'Type'.
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Type));
    if(strncmp(text, "L", 1) == 0) filter.fclass = Lowpass;
    else if(strncmp(text, "H", 1) == 0) filter.fclass = Highpass;
    else if(strncmp(text, "B", 1) == 0) filter.fclass = Bandpass;

    /// Read 'Type2': shape of response.
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Type2));
    if(strncmp(text, "Be", 2) == 0) filter.sclass = Bessel;
    else if(strncmp(text, "Bu", 2) == 0) filter.sclass = Butterworth;
    else if(strncmp(text, "C", 1) == 0) filter.sclass = Chebyshev;

    /// Read realisation.
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Circuit));
    if(strncmp(text, "S", 1) == 0) {
    	filter.cclass = SallKey;
    	gtk_widget_hide(LabelS);
    }
    else if(strncmp(text, "R", 1) == 0) {
    	filter.cclass = Rauch;
    	gtk_widget_hide(LabelS);
    }
    else if(strncmp(text, "D", 1) == 0) {
        filter.cclass = ZDomain;
        gtk_widget_show(LabelS);
    }

    /// Read frequency.
    text = gtk_entry_get_text(GTK_ENTRY(Freq));
    if(strlen(text)) n = sscanf(text, "%lf", &filter.frequency);
    if((n == 0) || (filter.frequency <= 0.0)) {
        filter.frequency = 1.0;
    }

    /// read (prototype) filter order.
    text = gtk_entry_get_text(GTK_ENTRY(Order));

    if(strlen(text)) sscanf(text, "%d", &filter.poles);
    if((n == 0) || (filter.poles < 2) || ((filter.poles < 3) && (filter.fclass != Bandpass))) {
        filter.poles = 3;
    }

    /// Read Ripple in dB.
    text = gtk_entry_get_text(GTK_ENTRY(Ripple));
    if(strlen(text)) sscanf(text, "%lf", &filter.ripple);
    if((n == 0) || (filter.ripple < 0.0)) {
        filter.ripple = 1.0;
    }

    /// Read Bandwidth.
    text = gtk_entry_get_text(GTK_ENTRY(Bandwidth));
    if(strlen(text)) sscanf(text, "%lf", &filter.bandwidth);
    if((n == 0) || (filter.bandwidth <= 2)) {
        filter.bandwidth = filter.frequency/2.0;
    }

    /// Read Sampling frequency; must be more than 2*frequency above.
    text = gtk_entry_get_text(GTK_ENTRY(SamplingFreq));
    if(strlen(text)) sscanf(text, "%lf", &filter.samplingfreq);

    filter.Calculate( );
    bode_calc(filter);
    filter.step_calc( );
    xyscale = scale(filter);
    
    gtk_widget_show(Area1);
        
    gtk_window_get_size((GtkWindow *)window2, &w, &h);

    if(strncmp(Dstring, "Syn", 3) == 0) {
        gtk_widget_hide(window2);
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
        cout << "\n";
        for(i=0; i<filter.poles/2; i++) {
            cout << "Stage " << i << ": T = " << filter.st[i].T << "; q = " << filter.st[i].q << "\n";
        }
        if(filter.poles % 2) cout << "Simple lag: " << "Tau = " << filter.tau << "\n";
    } else gtk_widget_show(window2);
        
    gtk_adjustment_set_value (GTK_ADJUSTMENT(scale_adj1), 0.0);
    gtk_widget_queue_draw(window2);

}



/// Callback function warns if certain comboboxes are changed.
static gboolean combo_changed(GtkWidget *area, cairo_t *cr, gpointer user_data)
{
    gchar *text;
#if VERBOSE
    cout << "combo_changed\n";
#endif
    /// Circuit type - is it Digital?
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Circuit));
    if(strncmp(text, "D", 1) == 0) {
        gtk_widget_show(LabelS);
        gtk_widget_show(SamplingFreq);
    } else {
        gtk_widget_hide(LabelS);
        gtk_widget_hide(SamplingFreq);
    }
    
    /// Filter type - is it Bandpass?
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Type));
    if(strncmp(text, "B", 1) == 0) {
        gtk_widget_show(LabelB);
        gtk_widget_show(Bandwidth);
    } else {
        gtk_widget_hide(LabelB);
        gtk_widget_hide(Bandwidth);
    }
    
    /// Filter response - is it Chebyshev?
    text = (gchar *)gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(Type2));
    if(strncmp(text, "C", 1) == 0) {
        gtk_widget_show(LabelR);
        gtk_widget_show(Ripple);
    } else {
        gtk_widget_hide(LabelR);
        gtk_widget_hide(Ripple);
    }
    
    gtk_widget_hide(Area1);
    gtk_widget_hide(cct_diagram);
    
    gtk_widget_queue_draw(window1);
    return(true);
}



//gboolean slider_changed(GtkWidget *adj, cairo_t *cr, gpointer user_data)
gdouble slider_changed(GtkAdjustment *adj)
{
    slider_position = gtk_adjustment_get_value((GtkAdjustment *)scale_adj1);
    gtk_widget_queue_draw(window2);
    return slider_position;
}



static gboolean on_draw(GtkWidget *area, cairo_t *cr, gpointer user_data)
//void on_draw(void)
{
void Draw_poles(cairo_t *, gdouble, gdouble);
void DrawStep(cairo_t *, gdouble, gdouble);
void DrawBode(GtkWidget *area, cairo_t *, gdouble, gdouble);

#if VERBOSE
    cout << "cr = " << (long)cr << "\n";
    g_print("<On_draw>\n");
#endif

    gtk_widget_show_all(window2);
        
    if(strncmp(Dstring, "S-p", 2) == 0) {
        gtk_widget_hide(scale1);
        Draw_mode = Splane;
        Draw_poles(cr, 250.0, 250.0);
    }
    if(strncmp(Dstring, "T-d", 2) == 0) {
//        gtk_adjustment_set_value (GTK_ADJUSTMENT(scale_adj1), 0.0);
        gtk_widget_show(scale1);
        Draw_mode = Step;
        DrawStep(cr, 250, 250);
    }
    if(strncmp(Dstring, "F-d", 2) == 0) {
//        gtk_adjustment_set_value (GTK_ADJUSTMENT(scale_adj1), (double)FSTEPS/2.0);
        gtk_widget_show(scale1);
        Draw_mode = Bode;
        DrawBode(area, cr, 250, 250);
    }
        
#if VERBOSE
    g_print("</On_draw>\n");
#endif
    return(TRUE);
}



/// Define a method  to be called if the close icon is clicked.
gint callback::quit(GtkWidget *Widget, GdkEvent *event, gpointer data)
{
//  g_print("'Quit' callback\n");

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
void DrawBode(GtkWidget *area, cairo_t *cr, gdouble x, gdouble y)
{
    extern double n_to_freq(int n, double f1, int nmax=FSTEPS, int decades=2);
    char str[80], string1[80];
    gdouble ftest, gain;
    gint i;
    gdouble xx1, yy1, xx2, yy2, xx, yy;
    gdouble width, height;
    double a, f;
    GdkRectangle da;            /* GtkDrawingArea size */
    gdouble dx = 5.0, dy = 5.0; /* Pixels between each point */
    gdouble clip_x1 = 0.0, clip_y1 = 0.0, clip_x2 = 0.0, clip_y2 = 0.0;
    gdouble left, right, top, bottom;
    GdkWindow *window = gtk_widget_get_window(Area1);
    assert(window != NULL);
    gtk_window_set_title (GTK_WINDOW(window2),  (gchar *)"Bode diagram");
        
#if VERBOSE
    cout << "DrawBode: cr = " << (long)cr << "\n";
    cout << "Size request\n";    
#endif
    gtk_widget_set_size_request(window2, 750, 750);
            
    /* Determine GtkDrawingArea dimensions. x, y is the position relative to the parent window. Width and height are obvious. */
    gdk_window_get_geometry((GdkWindow*)window, &da.x, &da.y, &da.width, &da.height);

#if VERBOSE
    cout << "Set line width\n";
#endif
    cairo_set_line_width(cr, 0.02);
        
#if VERBOSE
    cout << "set source rgb\n";
#endif
    /* Draw on a black background */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
#if VERBOSE
    cout << "stroke\n";
#endif
    cairo_stroke(cr);

#if VERBOSE
    cout << "translate\n";
#endif
    /* Change the transformation matrix */
    /* translate points in user space to the device space. */
//    cairo_translate(cr, 7*da.width / 8, da.height / 2);
    cairo_translate(cr, 1, 1);
    /* Scales the user space into the device space. */
        
#if VERBOSE
    cout << "scale\n";
#endif
    cairo_scale(cr, ZOOM_X, ZOOM_Y);

    /* Determine the data points to calculate (ie. those in the clipping zone) */
//    cairo_device_to_user_distance(cr, &dx, &dy);
#if VERBOSE
    cout << "extents\n";
#endif
    cairo_clip_extents(cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

#if VERBOSE
    g_print("<DrawBode>\n");
#endif

// Let's pick colour blue for the foreground.
    cairo_set_source_rgb(cr, 0.5, 0.5, 1.0);
        
#if VERBOSE
    cout << "select font\n";
#endif
    cairo_select_font_face(cr, "NimbusSans-Regular", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
#if VERBOSE
    cout << "set font size\n";
#endif
    cairo_set_font_size(cr, 0.075);
        
    width = clip_x2 - clip_x1;
    left = clip_x1 + width / 100.0;
    width = clip_x2 - left;
    height = clip_y1 - clip_y2;
    yy = (2.0 * clip_y1 + clip_y2) / 3.0;
#if VERBOSE
    cout << "draw lines\n";
#endif
#if 0
// Diagonal line test.
    cairo_move_to(cr, left, clip_y1);
    cairo_line_to(cr, clip_x2, clip_y2);
#endif
    cairo_move_to(cr, left, clip_y1);
    cairo_line_to(cr, left, clip_y2);
    cairo_move_to(cr, left, yy);
    cairo_line_to(cr, clip_x2, yy);

    cairo_stroke(cr);

/// Draw the frequency response curve.
    for(i=1; i<FSTEPS; i++) {
        xx1 = left + (i-1) * width / FSTEPS;
        yy1 = yy + 0.5*log10(filter.freq_resp[i-1]) * 0.6*height;
        xx2 = left + i * width / FSTEPS;
        yy2 = yy + 0.5*log10(filter.freq_resp[i]) * 0.6*height;
        if(i == 1) cairo_move_to(cr, xx1, yy1);
        cairo_line_to(cr, xx2, yy2);
    }
    cairo_stroke(cr);
        
    cairo_select_font_face(cr, "NimbusSans-Regular", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 0.2);
    cairo_move_to(cr, left, 0.9*clip_y1+0.1*clip_y2);
    ftest = filter.fmax * slider_position / 1000.0;
    gain = filter.freq_resp[(int)slider_position];
    strcpy(string1, "No data");
    sprintf(string1, "%0.3f Hz: gain = %0.3f", ftest, gain);
//    sprintf(string1, "%0.2f: ", slider_position);
    cairo_set_source_rgb(cr, 0.9, 0.9, 1.0);
    cairo_show_text(cr, string1);
    cairo_stroke(cr);
        
/// Draw frequency marker against bode curve.
    xx1 = left + width * slider_position / 1000.0;
    yy1 = yy + 0.5*log10(filter.freq_resp[(int)slider_position]) * 0.6*height;
    yy2 = yy1 - 0.05 * height;
    cairo_move_to(cr, xx1-0.04*width, yy1);
    cairo_line_to(cr, xx1+0.04*width, yy1);
    yy1 += 0.05 * height;
    cairo_move_to(cr, xx1, yy1);
    cairo_line_to(cr, xx1, yy2);
        
    cairo_stroke(cr);

#if VERBOSE
    g_print("</DrawBode>\n");
#endif
}



/// Draw the s-plane diagram.
void Draw_poles(cairo_t *cr, gdouble x, gdouble y)
{

    GdkRectangle da;            /* GtkDrawingArea size */
    gdouble dx = 5.0, dy = 5.0; /* Pixels between each point */
    gdouble clip_x1 = 0.0, clip_y1 = 0.0, clip_x2 = 0.0, clip_y2 = 0.0;
    gdouble left, right, top, bottom;
    gint i;
    gchar string1[64];
    gdouble xx1, yy1, xx2, yy2, xx, yy;
    gdouble width, height, xyscale;
    GdkWindow *window = gtk_widget_get_window(Area1);
    gdouble scale(TFilter);
    assert(window != NULL);
    gtk_window_set_title (GTK_WINDOW(window2),  (gchar *)"S-plane");

#if VERBOSE
    cout << "DrawBode: cr = " << (long)cr << "\n";
    cout << "Size request\n"; 
#endif   
    gtk_widget_set_size_request(window2, 400, 750);
            
    /* Determine GtkDrawingArea dimensions. x, y is the position relative to the parent window. Width and height are obvious. */
    gdk_window_get_geometry((GdkWindow*)window, &da.x, &da.y, &da.width, &da.height);

#if VERBOSE
    cout << "Set line width\n";
#endif
    cairo_set_line_width(cr, 0.02);
        
#if VERBOSE
    cout << "set source rgb\n";
#endif
    /* Draw on a blue background */
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.8);
    cairo_paint(cr);
#if VERBOSE
    cout << "stroke\n";
#endif
    cairo_stroke(cr);

#if VERBOSE
    cout << "translate\n";
#endif
    /* translate points in user space to the device space. */
    cairo_translate(cr, 1, 1);
    /* Scales the user space into the device space. */
#if VERBOSE
    cout << "scale\n";
#endif
    cairo_scale(cr, ZOOM_X, ZOOM_Y);

#if VERBOSE
    cout << "extents\n";
#endif
    cairo_clip_extents(cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

#if VERBOSE
    g_print("<Draw_poles>\n");
#endif

// Let's pick colour blue for the foreground.
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        
#if VERBOSE
    cout << "select font\n";
#endif

    cairo_select_font_face(cr, "NimbusSans-Regular", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
#if VERBOSE
    cout << "set font size\n";
#endif
    cairo_set_font_size(cr, 0.25);
        
    width = clip_x2 - clip_x1;
    left = clip_x1 + width / 100.0;
    width = clip_x2 - left;
    height = clip_y2 - clip_y1;
    yy = (2.0 * clip_y1 + clip_y2) / 3.0;
    cairo_move_to(cr, left+0.9*width, clip_y1);
    cairo_line_to(cr, left+0.9*width, clip_y2);
    cairo_move_to(cr, left, clip_y1+0.5*height);
    cairo_line_to(cr, left+width, clip_y1+0.5*height);
    cairo_stroke(cr);

    xyscale = scale(filter);
    for(i=0; i<filter.poles; i++) {
        xx = filter.st[i].pole.real() * 0.5 * width / xyscale + left+0.9*width;
        yy = filter.st[i].pole.imag() * 0.5*height / xyscale + 0.5*height;
        cairo_move_to(cr, xx-0.025*width, yy);
        cairo_line_to(cr, xx+0.025*width, yy);
        cairo_move_to(cr, xx, yy-0.025*height);
        cairo_line_to(cr, xx, yy+0.025*height);
        cairo_move_to(cr, xx-0.25*width, yy);
        sprintf(string1, "%0.2f + i*%0.2f ", filter.st[i].pole.real(), filter.st[i].pole.imag());
        cairo_show_text(cr, string1);
        cairo_move_to(cr, left+0.025*width, (i+1) * height / 20.0);
        if(i < filter.poles/2) {
            if(filter.st[i].T < 0.001) sprintf(string1, "T = %0.6f q = %0.2f ", filter.st[i].T, filter.st[i].q);
            else if(filter.st[i].T < 0.01) sprintf(string1, "T = %0.5f q = %0.2f ", filter.st[i].T, filter.st[i].q);
            else sprintf(string1, "T = %0.4f q = %0.2f ", filter.st[i].T, filter.st[i].q);
            cairo_show_text(cr, string1);
        }
    }
    if(filter.poles % 2)  {
        cairo_move_to(cr, left+0.025*width, (filter.poles / 2 + 1) * height / 20.0);
        if(filter.tau < 0.001) sprintf(string1, "Tau = %0.6f ", -filter.tau);
        else if(filter.tau < 0.01) sprintf(string1, "Tau = %0.5f ", -filter.tau);
        else sprintf(string1, "Tau = %0.4f ", -filter.tau);
        cairo_show_text(cr, string1);
    }
    cairo_stroke(cr);
    
    if(filter.zeroes) {
	// Show the zeroes.
	    cairo_move_to(cr, left+0.8*width, clip_y1+0.5*height-0.025*width);
	    sprintf(string1, "%d zeroes", filter.zeroes);
	    cairo_show_text(cr, string1);
	// Draw the zero as a circle.
	    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	    cairo_move_to(cr, left+0.9*width+0.025*width, clip_y1+0.5*height);
	    cairo_arc(cr, left+0.9*width, clip_y1+0.5*height, 0.025*width, 0.0, 2.0*M_PI);
    }
    cairo_stroke(cr);
}



/// Draw the filter's step response.
void DrawStep(cairo_t *cr, gdouble x, gdouble y)
{
    char str[80], string1[80];
    gint i, w, h;
    gdouble yy, xx1, yy1, xx2, yy2;
    gdouble width, height;
    gdouble t, v;
    GdkRectangle da;            /* GtkDrawingArea size */
    gdouble dx = 5.0, dy = 5.0; /* Pixels between each point */
    gdouble clip_x1 = 0.0, clip_y1 = 0.0, clip_x2 = 0.0, clip_y2 = 0.0;
    gdouble left, right, top, bottom;
    GdkWindow *window = gtk_widget_get_window(Area1);
    assert(window != NULL);
    gtk_window_set_title (GTK_WINDOW(window2),  (gchar *)"Step response");

#if VERBOSE
    cout << "DrawStep: cr = " << (long)cr << "\n";
    cout << "Size request\n";    
#endif
    gtk_widget_set_size_request(window2, 750, 750);
            
    /* Determine GtkDrawingArea dimensions. x, y is the position relative to the parent window. Width and height are obvious. */
    gdk_window_get_geometry((GdkWindow*)window, &da.x, &da.y, &da.width, &da.height);

#if VERBOSE
    cout << "Set line width\n";
#endif
    cairo_set_line_width(cr, 0.02);
        
#if VERBOSE
    cout << "set source rgb\n";
#endif
    /* Draw on a black background */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);
#if VERBOSE
    cout << "stroke\n";
#endif
    cairo_stroke(cr);

#if VERBOSE
    cout << "translate\n";
#endif
    /* Change the transformation matrix */
    /* translate points in user space to the device space. */
    cairo_translate(cr, 1, 1);
    /* Scales the user space into the device space. */
        
#if VERBOSE
    cout << "scale\n";
#endif
    cairo_scale(cr, ZOOM_X, ZOOM_Y);

    /* Determine the data points to calculate (ie. those in the clipping zone) */
//    cairo_device_to_user_distance(cr, &dx, &dy);
#if VERBOSE
    cout << "extents\n";
#endif
    cairo_clip_extents(cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

#if VERBOSE
    g_print("<DrawBode>\n");
#endif

// Let's pick colour blue for the foreground.
    cairo_set_source_rgb(cr, 0.5, 0.5, 1.0);
        
#if VERBOSE
    cout << "select font\n";
#endif
    cairo_select_font_face(cr, "NimbusSans-Regular", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
#if VERBOSE
    cout << "set font size\n";
#endif
    cairo_set_font_size(cr, 0.075);
        
    width = clip_x2 - clip_x1;
    height = clip_y2 - clip_y1;
    left = clip_x1 + width / 100.0;
    yy = (2.0 * clip_y1 + clip_y2) / 3.0;
#if VERBOSE
    cout << "draw axes\n";
#endif
#if 0
// Diagonal line test.
    cairo_move_to(cr, left, clip_y1);
    cairo_line_to(cr, clip_x2, clip_y2);
#endif
        
    /// Draw suitable axes.
    cairo_move_to(cr, left, clip_y1);
    cairo_line_to(cr, left, clip_y2);       // Vertical axis.
    switch(filter.fclass) {
        case Lowpass:
        yy = (0.2 * clip_y1 + 0.8 * clip_y2);
        break;
        case Bandpass:
        yy = (0.5 * clip_y1 + 0.5 * clip_y2);
        break;
        case Highpass:
        yy = (0.4 * clip_y1 + 0.6 * clip_y2);
        break;
    }
    cairo_move_to(cr, left, yy);
    cairo_line_to(cr, clip_x2, yy);     // Horizontal axis.
    cairo_stroke(cr);

    filter.step_calc( );
        
    /// Draw the step response curve.
    for(i=1; i<TSTEPS; i++) {
        xx1 = left + (i-1) * width / TSTEPS;
        xx2 = left + i * width / TSTEPS;
        switch(filter.fclass) {
            case Lowpass:
            yy1 = (0.2 * clip_y1 + 0.8 * clip_y2) - filter.step_resp[i-1] * 0.6*height;
            yy2 = (0.2 * clip_y1 + 0.8 * clip_y2) - filter.step_resp[i] * 0.6*height;
            break;
            case Bandpass:
            yy1 = (0.5 * clip_y1 + 0.5 * clip_y2) - filter.step_resp[i-1] * 0.4*height;
            yy2 = (0.5 * clip_y1 + 0.5 * clip_y2) - filter.step_resp[i] * 0.4*height;
            break;
            case Highpass: 
            yy1 = (0.4 * clip_y1 + 0.6 * clip_y2) - filter.step_resp[i-1] * 0.5*height;
            yy2 = (0.4 * clip_y1 + 0.6 * clip_y2) - filter.step_resp[i] * 0.5*height;
            default: break;
        }
        if(i == 1) cairo_move_to(cr, xx1, yy1);
        cairo_line_to(cr, xx2, yy2);
    }
    cairo_stroke(cr);

/// Draw frequency marker against bode curve.
    t = filter.tmax * slider_position / TSTEPS;
    switch(filter.fclass) {
        case Lowpass:
        yy1 = (0.2 * clip_y1 + 0.8 * clip_y2) - filter.step_resp[(int)slider_position] * 0.6*height;
        break;
        case Bandpass:
        yy1 = (0.5 * clip_y1 + 0.5 * clip_y2) - filter.step_resp[(int)slider_position] * 0.4*height;
        break;
        case Highpass: 
        yy1 = (0.4 * clip_y1 + 0.6 * clip_y2) - filter.step_resp[(int)slider_position] * 0.5*height;
        default: break;
    }
    xx1 = left + width * slider_position / 1000.0;
    cairo_move_to(cr, xx1-0.04*width, yy1);
    cairo_line_to(cr, xx1+0.04*width, yy1);
    cairo_move_to(cr, xx1, yy1-0.05*height);
    cairo_line_to(cr, xx1, yy1+0.05*height);
        
    cairo_stroke(cr);

    cairo_select_font_face(cr, "NimbusSans-Regular", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 0.2);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, left, 0.9*clip_y1+0.1*clip_y2);
    strcpy(string1, "No data");
    sprintf(string1, "t=%0.3f: f(t)=%0.3f", slider_position * filter.tmax / 1000.0, filter.step_resp[(int)slider_position]);
    cairo_show_text(cr, string1);
    cairo_stroke(cr);

#if 0
/// List the stage Ts and qs etc.
    if(filter.type != '0') for(i=0; i<filter.poles; i+=2) {
        if(i == filter.poles - 1) sprintf(str, "Tau = %1.3lf ", filter.st[i/2].T);
        else sprintf(str, "T = %1.3lf, q = %1.3lf", filter.st[i/2].T, filter.st[i/2].q);
        gdk_draw_string(widget->window, Font1, widget->style->black_gc, w / 10, 20 + i * 15, str);
    }
#if VERBOSE
    g_print("</DrawStep>\n");
#endif
#endif
}



/** Draw a rectangle on the screen, and add the axes, poles and
the Ts and qs. */
void
DrawMap(cairo_t *cr, gdouble x, gdouble y)
{
#if 0
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
#endif
} 



/** Define a function to be called if the window is re-exposed. */
static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event)
{
    return(TRUE);
}



/** If a button is pressed, do what? */
static gboolean
button_press_event( GtkWidget *widget, GdkEventButton *event )
{
    return TRUE;
}



/** Redraw the diagram if pointer moves. */
static gboolean
motion_notify_event( GtkWidget *widget, GdkEventMotion *event )
{
    return TRUE;
} 



/*!
'configure_event( )' redraws the window during resizing.
*/
static gboolean
configure_event( GtkWidget *widget, GdkEventConfigure *event )
{
    return TRUE;
} 



bool Check_band(void)
{
    return(TRUE);
}



/*!
'Type_event' call-back function to process changed data in filter class combo-box.
*/
void Type_event(GtkComboBox *widget, gpointer data)
{
    gtk_widget_hide(cct_diagram);
}



/*!
'Type2_event' call-back function to process changed data in filter class combo-box.
*/
void Type2_event(GtkComboBox *widget, gpointer data)
{

}




/*!
'Circuit_event' call-back function to process changed data in filter class combo-box.
*/
void Circuit_event(GtkComboBox *widget, gpointer data)
{
}



/*!
'Freq_callback' call-back function to process changed data in frequency combo-box.
*/
void Freq_callback(GtkWidget *widget, GtkWidget *textbox)
{
#if VERBOSE
    puts("Freq_callback\n");
#endif
}



/*!
'Atten_callback' call-back function to process changed data in attenuation combo-box.
*/
void Atten_callback(GtkWidget *widget, GtkWidget *textbox)
{
}



/*!
'SFreq_callback' call-back function to process changed data in frequency combo-box.
*/
void SFreq_callback(GtkWidget *widget, GtkWidget *textbox)
{
}



/*!
'q_callback' call-back function to process changed data in filter 'q' combo-box.
*/
void q_callback(GtkWidget *widget, GtkWidget *textbox)
{
}



/*!
'order_callback' call-back function to process changed data in filter 'order' combo-box.
*/
void order_callback(GtkWidget *widget, GtkWidget *textbox)
{
}



/*!
'ripple_callback' call-back function to process changed data in filter 'ripple' combo-box.
*/
void ripple_callback(GtkWidget *widget, GtkWidget *textbox)
{
}



/*!
'bandwidth_callback' call-back function to process changed data in filter 'ripple' combo-box.
*/
void bandwidth_callback(GtkWidget *widget, GtkWidget *textbox)
{
}




/** 'time_event' reads slider value at regular intervals. This avoids large
numbers of redrawings as the slider is moved.
*/
gboolean time_event(GtkWidget *widget)
{
    return TRUE;
}




int main(int argc, char *argv[ ])
{
    puts("'rbfilter' version ");
    puts(VERSION);
    puts("Copyright (c) Roger Burghall 2013..2022\n");

    ofstream logfile;
    logfile.open("./rbfilter.log", ios::trunc);
    logfile << "rbfilter log file.\n******************\n";
    logfile.close( );

    GtkWidget *hbox, *vbox_draw, *vbox_main;
    GList *glist, *glist1;
    GtkWidget *vbox_cct;
    GtkWidget *da;

    gtk_init(&argc, &argv);

/// Create a window for the controls.
    window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    assert(window1 != NULL);
    gtk_window_set_default_size(GTK_WINDOW(window1), WIDTH, HEIGHT);
    gtk_window_set_title(GTK_WINDOW(window1), "rbfilter controls");
    g_signal_connect(G_OBJECT(window1), "destroy", gtk_main_quit, NULL);
        
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 480, 480);
    cairo_t *cr = cairo_create(surface);

/// Create another window for graphs etc.
    window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    assert(window2 != NULL);
    g_signal_connect(G_OBJECT(window2), "destroy", gtk_main_quit, NULL);
    gtk_window_set_position ((GtkWindow *)window2, GTK_WIN_POS_CENTER);
            
    gtk_widget_set_size_request(GTK_WIDGET(window2), 600, 650);
    gtk_window_set_title ((GtkWindow *)window2,  (gchar *)"rbfilter graphics");
            
    vbox_draw = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

/// Create a drawing area which must receive extra space when window is enlarged.
    Area1 = gtk_drawing_area_new ( );
        
/// Create a scale
    scale_adj1 = gtk_adjustment_new (FSTEPS/2.0, 0.0, (double)FSTEPS, 10.0, 10.0, 10.0);
    scale1 = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(scale_adj1));
    gtk_scale_set_draw_value ((GtkScale*)scale1, FALSE);

/// Put a vbox in the window and the scale in the vbox.    
    gtk_container_add(GTK_CONTAINER(window2), vbox_draw);
    gtk_box_pack_start(GTK_BOX(vbox_draw), scale1, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(scale1), "value_changed", G_CALLBACK(slider_changed), NULL);
    gtk_container_add(GTK_CONTAINER(vbox_draw), Area1);
    gtk_widget_set_size_request(GTK_WIDGET(Area1), XSIZE, YSIZE);
    g_signal_connect(G_OBJECT(Area1), "draw", G_CALLBACK(on_draw), NULL);
    gtk_widget_set_size_request(Area1, 750, 750);
    gtk_widget_show_all(window2);
                
/// Draw a third window, for showing the circuit diagrams
    cct_diagram = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    assert(cct_diagram != NULL);

/// Set the size, position and title of the circuit diagram window.
    gtk_window_set_position ((GtkWindow *)cct_diagram, GTK_WIN_POS_NONE);
    gtk_widget_set_size_request(GTK_WIDGET(cct_diagram), 600, 650);
    gtk_window_set_title ((GtkWindow *)cct_diagram,  (gchar *)"Realisation");

    GtkWidget *cct_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    image = gtk_image_new( );
    gtk_container_add(GTK_CONTAINER(cct_diagram), cct_box);
    gtk_container_add(GTK_CONTAINER(cct_box), image);


/// Remove close icon from the realisation window.
    gtk_window_set_deletable((GtkWindow *)cct_diagram, false);

/// Position the control window to the left of the screen. GTK2 could tell us the dimensions of \
the screen; it seems GTK3 can't which is completely *!~#.
/// Position the control window to the left of the screen.
    gtk_window_move((GtkWindow *)window1, 100, 100);

/// Create a 'vbox' and place it in the application.
    vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_container_add(GTK_CONTAINER(window1), vbox_main);
        
    /// Create an 'hbox' and put it in the 'vbox'
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_add (GTK_CONTAINER(vbox_main), hbox);
        
    /// Create button 'vbox'es and place them in the 'hbox'.
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_box_pack_start (GTK_BOX (hbox), vbox1, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
        
/// Put data entry boxes in 'vbox1':-
/// Combobox permitting only preset entries for filter class
    GtkWidget *Label = gtk_label_new ("Class");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), FALSE, FALSE, 1);
    Type = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type, -1, (gchar *)"Low-pass");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type, -1, (gchar *)"High-pass");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type, -1, (gchar *)"Band-pass");
    gtk_container_add(GTK_CONTAINER(vbox1), Type);
    gtk_combo_box_set_active(GTK_COMBO_BOX(Type), 0);
    g_signal_connect((GtkComboBoxText *)(Type), "changed", G_CALLBACK(combo_changed), NULL);

    Label = gtk_label_new ("Response");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), FALSE, FALSE, 1);

    Type2 = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type2, -1, (gchar *)"Bessel");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type2, -1, (gchar *)"Butterworth");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Type2, -1, (gchar *)"Chebyshev");
    gtk_container_add(GTK_CONTAINER(vbox1), Type2);
    gtk_combo_box_set_active(GTK_COMBO_BOX(Type2), 0);
    g_signal_connect((GtkComboBoxText *)(Type2), "changed", G_CALLBACK(combo_changed), NULL);


/// Combobox permitting only preset entries for filter class
    Label = gtk_label_new ("Circuit");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), FALSE, FALSE, 1);

    Circuit = gtk_combo_box_text_new();
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Circuit, -1, (gchar *)"Sallen and Key");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Circuit, -1, (gchar *)"Rauch");
    gtk_combo_box_text_insert_text((GtkComboBoxText *)Circuit, -1, (gchar *)"Discrete");
    gtk_container_add(GTK_CONTAINER(vbox1), Circuit);
    gtk_combo_box_set_active(GTK_COMBO_BOX(Circuit), 0);
    g_signal_connect((GtkComboBoxText *)(Circuit), "changed", G_CALLBACK(combo_changed), NULL);


    /// Combobox permitting non-standard entries for cut-off frequency
    Label = gtk_label_new ("Freq");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
        
    Freq = gtk_entry_new( );
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Freq), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)Freq, "1.0");

    g_signal_connect(Freq, "changed", G_CALLBACK(Freq_callback), Freq);
      
    // Version 2.11: remove Atten input box. 
    // Combobox permitting non-standard entries for cut-off attenuation
    Label = gtk_label_new("Atten (dB)");
//    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
//    gtk_widget_show(GTK_WIDGET(Label));
    Atten = gtk_entry_new( );
//    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Atten), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)Atten, "1.0");
    g_signal_connect(Atten, "changed", G_CALLBACK(Atten_callback), Atten);


    /// Entry box permitting non-standard entries for sampling frequency
    LabelS = gtk_label_new ("Sampling Freq");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelS), TRUE, FALSE, 0);
    SamplingFreq = gtk_entry_new( );
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(SamplingFreq), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)SamplingFreq, "100");
    g_signal_connect(SamplingFreq, "changed", G_CALLBACK(SFreq_callback), SamplingFreq);
         
    /// Combobox permitting non-standard entries for filter order
    Label = gtk_label_new ("Prototype order");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Label), TRUE, FALSE, 0);
    Order = gtk_entry_new( );
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Order), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)Order, "3");
    g_signal_connect(Order, "changed", G_CALLBACK(order_callback), Order);

    /// Combobox permitting non-standard entries for filter ripple (Chebyshev only)
    LabelR = gtk_label_new ("Ripple (dB)");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelR), TRUE, FALSE, 0);
    Ripple = gtk_entry_new( );
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Ripple), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)Ripple, "0.99");
    g_signal_connect(Ripple, "changed", G_CALLBACK(ripple_callback), Ripple);
     
    /// Combobox permitting non-standard entries for filter bandwidth (Bandpass only)
    LabelB = gtk_label_new ("Bandwidth");
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(LabelB), TRUE, FALSE, 0);
    Bandwidth = gtk_entry_new( );
    gtk_box_pack_start(GTK_BOX(vbox1), GTK_WIDGET(Bandwidth), TRUE, FALSE, 0);
    gtk_entry_set_text((GtkEntry *)Bandwidth, "1.0");
    g_signal_connect(Bandwidth, "changed", G_CALLBACK(bandwidth_callback), Bandwidth);

/// Create "S-p", "T-d" and "F-d" buttons and place them in 'vbox2'.
    sp_button = gtk_button_new_with_label("s-plane");
    t_button = gtk_button_new_with_label("Time");
    f_button = gtk_button_new_with_label("Freq");
    s_button = gtk_button_new_with_label("Synth");
    gtk_box_pack_start (GTK_BOX (vbox2), sp_button, TRUE, FALSE, 20);
    gtk_box_pack_start (GTK_BOX (vbox2), t_button, TRUE, FALSE, 20);
    gtk_box_pack_start (GTK_BOX (vbox2), f_button, TRUE, FALSE, 20);
    gtk_box_pack_start (GTK_BOX (vbox2), s_button, TRUE, FALSE, 20);
    /// Connect button signals
    gchar *Event2 = (gchar *)"clicked";
    gchar *Ctrl1 = (gchar *)"S-p\n";
    gchar *Ctrl2 = (gchar *)"T-d\n";
    gchar *Ctrl3 = (gchar *)"F-d\n";
    gchar *Ctrl4 = (gchar *)"Syn\n";
    gchar *Ctrl5 = (gchar *)"Z-t\n";
    g_signal_connect(sp_button, Event2, G_CALLBACK (callback::clicked), Ctrl1);
    g_signal_connect(t_button, Event2, G_CALLBACK (callback::clicked), Ctrl2);
    g_signal_connect(f_button, Event2, G_CALLBACK (callback::clicked), Ctrl3);
    g_signal_connect(s_button, Event2, G_CALLBACK (callback::clicked), Ctrl4);


    Draw_mode = Splane;

    gtk_widget_show_all(window1);
    gtk_widget_hide(LabelB);
    gtk_widget_hide(Bandwidth);
    gtk_widget_hide(LabelR);
    gtk_widget_hide(Ripple);
    gtk_widget_hide(LabelS);
    gtk_widget_hide(SamplingFreq);

    gtk_widget_show_all(window2);
    gtk_widget_hide(window2);
    
    gtk_widget_show_all(cct_diagram);
    gtk_widget_hide(cct_diagram);
    
    gtk_main( );

    return 0;
}



#if 0

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
#endif




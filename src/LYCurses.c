#include "HTUtils.h"
#include "tcp.h"
#include "LYCurses.h"
#include "LYStyle.h"
#include "LYUtils.h"
#include "LYGlobalDefs.h"
#include "LYSignal.h"
#include "LYClean.h"
#include "LYStrings.h"
#ifdef USE_SLANG
#include "LYCharSets.h"
#endif /* USE_SLANG */

#include "LYexit.h"
#include "LYLeaks.h"

#define FREE(x) if (x) {free(x); x = NULL;}

#ifdef VMS
#define DISPLAY "DECW$DISPLAY"
#else
#define DISPLAY "DISPLAY"
#endif /* VMS */

#if defined(VMS) && defined(__GNUC__)
#include <gnu_hacks.h>
#undef LINES
#undef COLS
#define LINES lines
#define COLS cols
extern int _NOSHARE(LINES);
extern int _NOSHARE(COLS);
#endif /* VMS && __GNUC__ */

#ifdef USE_COLOR_STYLE
#include "AttrList.h"
#include "LYHash.h"
int has_color = 0;
#endif

/*
 *  These are routines to start and stop curses and to cleanup
 *  the screen at the end.
 */

PRIVATE int dumbterm PARAMS((char *terminal));
BOOLEAN LYCursesON = FALSE;

#if USE_COLOR_TABLE || USE_SLANG
PRIVATE int Current_Attr;
#endif

#ifdef USE_SLANG
PUBLIC unsigned int Lynx_Color_Flags = 0;
PUBLIC BOOLEAN FullRefresh = FALSE;
PUBLIC int curscr = 0;
#ifdef SLANG_MBCS_HACK
PUBLIC int PHYSICAL_SLtt_Screen_Cols = 10;/* will be set by size_change - kw */
#endif /* SLANG_MBCS_HACK */

PUBLIC void LY_SLrefresh NOARGS
{
    if (FullRefresh) {
        SLsmg_touch_lines(0, LYlines);
	FullRefresh = FALSE;
    }
    SLsmg_refresh();

    return;
}

PUBLIC void LY_SLclear NOARGS
{
    SLsmg_gotorc (0, 0);
    SLsmg_erase_eos ();
}

#ifdef VMS
PUBLIC void VTHome NOARGS
{
    printf("\033[;H");

    return;
}
#endif /* VMS */

PUBLIC void lynx_add_attr ARGS1(
	int,		a)
{
    Current_Attr |= a;
    SLsmg_set_color(Current_Attr);
}

PUBLIC void lynx_sub_attr ARGS1(
	int,		a)
{
    Current_Attr &= ~a;
    SLsmg_set_color(Current_Attr);
}

PUBLIC void lynx_setup_colors NOARGS
{
    SLtt_set_color(0, NULL, "black", "white");
    SLtt_set_color(1, NULL, "blue", "white");	 /* bold */
    SLtt_set_color(2, NULL, "yellow", "blue");	 /* reverse */
    SLtt_set_color(4, NULL, "magenta", "white"); /* underline */
    /*
     *  The other objects are '|'ed together to get rest.
     */
    SLtt_set_color(3, NULL, "green", "white");	 /* bold-reverse */
    SLtt_set_color(5, NULL, "blue", "white");    /* bold-underline */
    SLtt_set_color(6, NULL, "red", "white");	 /* reverse-underline */
    SLtt_set_color(7, NULL, "magenta", "cyan");	 /* reverse-underline-bold */

    /*
     *  Now set monchrome attributes.
     */
    SLtt_set_mono(1, NULL, SLTT_BOLD_MASK);
    SLtt_set_mono(2, NULL, SLTT_REV_MASK);
    SLtt_set_mono(3, NULL, SLTT_REV_MASK | SLTT_BOLD_MASK);
    SLtt_set_mono(4, NULL, SLTT_ULINE_MASK);
    SLtt_set_mono(5, NULL, SLTT_ULINE_MASK | SLTT_BOLD_MASK);
    SLtt_set_mono(6, NULL, SLTT_ULINE_MASK | SLTT_REV_MASK);
    SLtt_set_mono(7, NULL, SLTT_ULINE_MASK | SLTT_BOLD_MASK | SLTT_REV_MASK);
}



PRIVATE void sl_suspend ARGS1(
	int,		sig)
{
#ifdef SIGSTOP
#ifndef VMS
    int r, c;
   
    lynx_enable_mouse (0);
    if (sig == SIGTSTP)
        SLsmg_suspend_smg();
    SLang_reset_tty();
    kill(getpid(),SIGSTOP);
#if SLANG_VERSION > 9929
    SLang_init_tty(-1, 0, 1);
#else
    SLang_init_tty(3, 0, 1);
#endif /* SLANG_VERSION > 9929 */
    signal(SIGTSTP, sl_suspend);
#ifndef _WINDOWS
    SLtty_set_suspend_state(1);
#endif
    if (sig == SIGTSTP)
        SLsmg_resume_smg();
    /*
     *  Get new window size in case it changed.
     */
    r = SLtt_Screen_Rows;
    c = SLtt_Screen_Cols;
    size_change(0);
    if ((r != SLtt_Screen_Rows) || (c != SLtt_Screen_Cols)) {
	recent_sizechange = TRUE;
    }
    lynx_enable_mouse (1);
#endif /* !VMS */
#endif /* SIGSTOP */
   return;
}
#endif /* USE_SLANG */

#if defined(USE_COLOR_STYLE) || defined(SLCS)
PRIVATE int last_styles[128];
PRIVATE int last_ptr=0;
#endif

#if defined(USE_COLOR_STYLE)
/* Ok, explanation of the USE_COLOR_STYLE styles.  The basic styles (ie non
 * HTML) are set the same as the SLANG version for ease of programming.  The
 * other styles are simply the HTML enum from HTMLDTD.h + 16.
 */
PUBLIC HTCharStyle displayStyles[DSTYLE_ELEMENTS];

/*
 * set a style's attributes - RP
 */
PUBLIC void setStyle ARGS4(int,style,int,color,int,cattr,int,mono)
{
	displayStyles[style].color=color;
	displayStyles[style].cattr=cattr;
	displayStyles[style].mono=mono;
}

PUBLIC void setHashStyle ARGS5(int,style,int,color,int,cattr,int,mono,char*,element)
{
        bucket* ds=&hashStyles[style];
        if (TRACE)
                fprintf(stderr, "CSS(SET): <%s> hash=%d, ca=%d, ma=%d\n", element, style, color, mono);
        ds->color=color;
        ds->cattr=cattr;
        ds->mono=mono;
        ds->code=style;
        ds->name=malloc(sizeof(char)*(strlen(element)+2));
        strcpy(ds->name, element);
}

/*
 * set the curses attributes to be color or mono - RP
 */
PRIVATE int LYAttrset ARGS3(WINDOW*,win,int,color,int,mono)
{
	if (TRACE)
		fprintf(stderr, "CSS:LYAttrset (%d, %d)\n", color, mono);
	if (has_color && color > -1)
	{
		wattrset(win,color);
#if 0
		//wbkgdset(win,color);
#endif
		return color;
	}
	if (mono > -1)
	{
		wattrset(win,mono);
#if 0
		//wbkgdset(win,mono);
#endif
		return mono;
	}
	wattrset(win,A_NORMAL);
#if 0
	//wbkgdset(win,A_NORMAL);
#endif
	return A_NORMAL;
}

PUBLIC void curses_w_style ARGS4(WINDOW*,win,int,style,int,dir,int,previous)
{
	int YP,XP;
	bucket* ds=&hashStyles[style];

	if (!ds->name)
	{
		if (TRACE)
		fprintf(stderr, "CSS.CS:Style %d not configured\n",style);
		return;
	}
	if (TRACE)
		fprintf(stderr, "CSS.CS:<%s%s> (%d)\n",(dir?"":"/"),ds->name,ds->code);

	getyx (win, YP, XP);

	if (style == s_normal && dir)
	{
		wattrset(win,A_NORMAL);
		if (win==stdscr) cached_styles[YP][XP]=s_normal;
		return;
	}

	switch (dir)
	{
		/* ABS_OFF is the same as STACK_OFF for the moment */
	case STACK_OFF:
		if (last_ptr)
			LYAttrset(win,last_styles[--last_ptr],-1);
		else
			LYAttrset(win,A_NORMAL,-1);
		return;

	case STACK_ON: /* remember the current attributes */
#ifndef _NCURSES_H
		last_styles[last_ptr++] = getattrs(stdscr);
#else
		last_styles[last_ptr++] = attr_get();
#endif
		/* don't cache style changes for active links */
		if (style != s_alink)
		{
			if (TRACE)
				fprintf(stderr, "CACHED: <%s> @(%d,%d)\n", ds->name, YP, XP);
			if (win==stdscr) cached_styles[YP][XP]=style;
			LYAttrset(win, ds->color, ds->mono);
		}
		else
		{
			LYAttrset(win, ds->color, ds->mono);
		}
		return;

	case ABS_ON: /* change without remembering the previous style */
		/* don't cache style changes for active links */
		if (style != s_alink)
		{
			if (TRACE)
				fprintf(stderr, "CACHED: <%s> @(%d,%d)\n", ds->name, YP, XP);
			if (win==stdscr) cached_styles[YP][XP]=style;
			LYAttrset(win, ds->color, ds->mono);
		}
		else
		{
			LYAttrset(win, ds->color, ds->mono);
		}
		return;
	}
}

/*
 * wrapper function to set on-screen styles - RP
 */
PUBLIC void wcurses_css ARGS3(WINDOW *,win,char*,name,int,dir)
{
	int try_again=1;
	while (try_again)
	{
		int tmpHash=hash_code(name);
		if (TRACE)
			fprintf(stderr, "CSSTRIM:trying to set [%s] style - ", name);
		if (tmpHash==NOSTYLE) {
			char *class=strrchr(name, '.');
			if (TRACE) fprintf(stderr, "undefined, trimming at %p\n", class);
			if (class)	*class='\0';
			else 		try_again=0;
		} else {
			if (TRACE) fprintf(stderr, "ok (%d)\n", hash_code(name));
			curses_w_style(win, hash_code(name), dir, 0);
			try_again=0;
		}
	}
}

PUBLIC void curses_css ARGS2(char *,name,int,dir)
{
	wcurses_css(stdscr, name, dir);
}

PUBLIC void curses_style ARGS3(int,style,int,dir,int,previous)
{
        curses_w_style(stdscr, style, dir, previous);
}

#if UNUSED
void attribute ARGS2(int,style,int,dir)
{
        curses_style(style, dir, 0);
}
#endif
#endif /* USE_COLOR_STYLE */

#if USE_COLOR_TABLE && defined(COLOR_CURSES)
/*
 * This block of code is designed to produce the same color effects using SVr4
 * curses as the slang library's implementation in this module.  That maps the
 * SGR codes into a 0-7 index into the color table, with special treatment for
 * backgrounds.  There's a bit of convoluted (but necessary) code handling the
 * special case of initialization before 'initscr()' is called.
 * 1997/1/19 - T.E.Dickey <dickey@clark.net>
 */
PRIVATE int lynx_uses_color;
PRIVATE int lynx_called_initscr;

PRIVATE struct {
    int fg, bg;
    chtype attr;
} lynx_color_cfg[] = {
    /*0*/ { COLOR_BLACK,   COLOR_WHITE, A_NORMAL}, /* A_NORMAL */
    /*1*/ { COLOR_BLUE,    COLOR_WHITE, A_NORMAL}, /* A_BOLD */
    /*2*/ { COLOR_YELLOW,  COLOR_BLUE,  A_BOLD},   /* A_REVERSE */
    /*3*/ { COLOR_GREEN,   COLOR_WHITE, A_NORMAL}, /* A_REVERSE | A_BOLD */
    /*4*/ { COLOR_MAGENTA, COLOR_WHITE, A_NORMAL}, /* A_UNDERLINE */
    /*5*/ { COLOR_BLUE,    COLOR_WHITE, A_NORMAL}, /* A_UNDERLINE | A_BOLD */
    /*6*/ { COLOR_RED,     COLOR_WHITE, A_NORMAL}, /* A_UNDERLINE | A_REVERSE */
    /*7*/ { COLOR_MAGENTA, COLOR_CYAN,  A_NORMAL}  /* A_UNDERLINE | A_BOLD | A_REVERSE */
};

/*
 * Hold the codes for color-pairs here until 'initscr()' is called.
 */
PRIVATE struct {
    int fg;
    int bg;
} lynx_color_pairs[25];

/*
 * Map the SGR attributes (0-7) into ANSI colors, modified with the actual BOLD
 * attribute we'll get 16 colors.
 */
PRIVATE void lynx_set_wattr ARGS1(WINDOW *, win)
{
    if (lynx_uses_color) {
	int code = 0;
	int attr = A_NORMAL;
	int offs = 1;
	static int have_underline = -1;
	static int no_color_video = -1;

	if (have_underline < 0) {
#ifndef DOSPATH
		have_underline = tigetstr("smul") != 0;
#else
                have_underline = 1;
#endif /* DOSPATH */
	}

	if (no_color_video < 0) {
		no_color_video = tigetnum("ncv");
	}

	if (Current_Attr & A_BOLD)
		code |= 1;
	if (Current_Attr & A_REVERSE)
		code |= 2;
	if (Current_Attr & A_UNDERLINE)
		code |= 4;
	attr = lynx_color_cfg[code].attr;

	/*
	 * FIXME:  no_color_video isn't implemented (97/4/14) in ncurses 4.x,
	 * but may be in SVr4 (which would make this redundant for the latter).
	 */
	if ((Current_Attr & A_BOLD) && !(no_color_video & 33)) {
		attr |= A_BOLD;
		offs = 17;
	}

	if ((Current_Attr & A_UNDERLINE) && !(no_color_video & 2)) {
		attr |= A_UNDERLINE;
		offs = 17;
	}

	attr |= COLOR_PAIR(code+offs);

	wattrset(win, attr);
    } else {
	wattrset(win, Current_Attr);
    }
}

PRIVATE void lynx_map_color ARGS1(int, n)
{
    int m;

    lynx_color_pairs[n+1].fg = lynx_color_cfg[n].fg;
    lynx_color_pairs[n+1].bg = lynx_color_cfg[n].bg;

    lynx_color_pairs[n+9].fg = lynx_color_cfg[n].fg;
    lynx_color_pairs[n+9].bg = lynx_color_cfg[0].bg;

    lynx_color_pairs[n+17].fg = lynx_color_cfg[n].bg;
    lynx_color_pairs[n+17].bg = lynx_color_cfg[n].bg;

    if (lynx_called_initscr) {
	for (m = 0; m <= 16; m += 8) {
	    init_pair(n+m+1,
		lynx_color_pairs[n+m+1].fg,
		lynx_color_pairs[n+m+1].bg);
	}
	if (n == 0)
	    bkgd(COLOR_PAIR(9));
    }
}

PUBLIC int lynx_chg_color ARGS3(
	int, color,
	int, fg,
	int, bg
	)
{
    if (color >= 0 && color < 8
     && fg >= 0 && fg < 16
     && bg >= 0 && bg < 16) {
	lynx_color_cfg[color].fg = fg & 7;
	lynx_color_cfg[color].bg = bg & 7;
	lynx_color_cfg[color].attr = (fg & 8) ? A_BOLD : A_NORMAL;
	lynx_map_color(color);
    } else {
	return -1;
    }
    return 0;
}

PUBLIC void lynx_set_color ARGS1(int, a)
{
    if (lynx_uses_color) {
	attrset(lynx_color_cfg[a].attr | COLOR_PAIR(a+1));
    }
}

PUBLIC void lynx_standout ARGS1(int, flag)
{
    if (flag)
	lynx_add_attr(A_REVERSE);
    else
	lynx_sub_attr(A_REVERSE);
}

PUBLIC void lynx_add_wattr ARGS2(WINDOW *, win, int, a)
{
    Current_Attr |= a;
    lynx_set_wattr(win);
}

PUBLIC void lynx_add_attr ARGS1(int, a)
{
    lynx_add_wattr (stdscr, a);
}

PUBLIC void lynx_sub_wattr ARGS2(WINDOW *, win, int, a)
{
    Current_Attr &= ~a;
    lynx_set_wattr(win);
}

PUBLIC void lynx_sub_attr ARGS1(int, a)
{
    lynx_sub_wattr (stdscr, a);
}

PRIVATE void lynx_init_colors NOARGS
{
    lynx_uses_color = FALSE;

    if (has_colors()) {
	int n, m;

	lynx_uses_color = TRUE;
	start_color();

	for (n = 0; n < sizeof(lynx_color_cfg)/sizeof(lynx_color_cfg[0]); n++) {
	    for (m = 0; m <= 16; m += 8) {
		init_pair(n+m+1,
			lynx_color_pairs[n+m+1].fg,
			lynx_color_pairs[n+m+1].bg);
	    }
	    if (n == 0)
		bkgd(COLOR_PAIR(9));
	}
    }
}

PUBLIC void lynx_setup_colors NOARGS
{
    int n;
    for (n = 0; n < 8; n++)
	lynx_map_color(n);
}
#endif /* USE_COLOR_TABLE */

#if defined (DJGPP) && !defined (USE_SLANG)
/*
 * Sorry about making a completely new function,
 * but the real one is messy! WB
 */
PUBLIC void start_curses NOARGS
{
	 static BOOLEAN first_time = TRUE;

	 if(first_time)
	 {
		  initscr();      /* start curses */
		  first_time = FALSE;
		  cbreak();
		  noecho();
		  keypad(stdscr, TRUE);
		  fflush(stdin);
		  fflush(stdout);
                  lynx_init_colors();
                  lynx_called_initscr = TRUE;

 /* Inform pdcurses that we're interested in knowing when mouse
    buttons are clicked.  Maybe someday pdcurses will support it.
 */
	 if (LYUseMouse)
	      lynx_enable_mouse (1);

	 } else sock_init();

	 LYCursesON = TRUE;
	 clear();
}
#else
PUBLIC void start_curses NOARGS
{
#ifdef USE_SLANG
    static int slinit;

    if (LYCursesON)
        return;

    if (slinit == 0) {
	SLtt_get_terminfo();
	if (Lynx_Color_Flags & SL_LYNX_USE_COLOR)
	    SLtt_Use_Ansi_Colors = 1;
	size_change(0);

	SLtt_add_color_attribute(4, SLTT_ULINE_MASK);
	SLtt_add_color_attribute(5, SLTT_ULINE_MASK);
	/*
	 *  If set, the blink escape sequence will turn on high
	 *  intensity background (rxvt and maybe Linux console).
	 */
	if (Lynx_Color_Flags & SL_LYNX_USE_BLINK)
	    SLtt_Blink_Mode = 1;	       
	else
	    SLtt_Blink_Mode = 0;
    }
    slinit = 1;
    Current_Attr = 0;
#ifndef VMS
#if SLANG_VERSION > 9929
    SLang_init_tty(-1, 0, 1);
#else
    SLang_init_tty(3, 0, 1);
#endif /* SLANG_VERSION > 9929 */
#endif /* !VMS */
    SLsmg_init_smg();
    SLsmg_Display_Eight_Bit = LYlowest_eightbit[current_char_set];
    if (SLsmg_Display_Eight_Bit > 191)
       SLsmg_Display_Eight_Bit = 191; /* may print ctrl chars otherwise - kw */
    SLsmg_Newline_Moves = -1;
    SLsmg_Backspace_Moves = 1;
#ifndef VMS
#ifndef _WINDOWS
   SLtty_set_suspend_state(1);
#endif /* _WINDOWS */
#ifdef SIGTSTP
   if (!no_suspend)
       signal(SIGTSTP, sl_suspend);
#endif /* SIGTSTP */
    signal(SIGINT, cleanup_sig);
#endif /* !VMS */

   lynx_enable_mouse (1);

#else /* Using curses: */

#ifdef VMS
    /*
     *  If we are VMS then do initscr() everytime start_curses()
     *  is called!
     */
    initscr();  /* start curses */
#else /* Unix: */
    static BOOLEAN first_time = TRUE;

    if (first_time) {
	/*
	 *  If we're not VMS then only do initsrc() one time,
	 *  and one time only!
	 */
	if (initscr() == NULL) {  /* start curses */
	    fprintf(stderr, 
	        "Terminal initialisation failed - unknown terminal type?\n");
#ifndef NOSIGHUP
            (void) signal(SIGHUP, SIG_DFL);
#endif /* NOSIGHUP */
            (void) signal(SIGTERM, SIG_DFL);
            (void) signal(SIGINT, SIG_DFL);
#ifdef SIGTSTP
	    if (no_suspend)
	        (void) signal(SIGTSTP,SIG_DFL);
#endif /* SIGTSTP */
	    exit (-1);
	}
#ifdef USE_COLOR_STYLE
	has_color = has_colors();
	if (has_color)
		start_color();
	parse_userstyles();	
#endif
        first_time = FALSE;
#if USE_COLOR_TABLE
	lynx_init_colors();
	lynx_called_initscr = TRUE;
#endif /* USE_SLANG */
    }
#endif /* VMS */

    /* nonl();   */ /* seems to slow things down */

#ifdef VMS
    crmode();
    raw();
#else
#if HAVE_CBREAK
    cbreak();
#else
    crmode();
#endif /* HAVE_CBREAK */
    signal(SIGINT, cleanup_sig);
#endif /* VMS */

    noecho();

#if defined(HAVE_KEYPAD)
    keypad(stdscr,TRUE);
#endif /* HAVE_KEYPAD */

   lynx_enable_mouse (1);

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
#endif /* USE_SLANG */

#ifdef _WINDOWS
    clear();
#endif

    LYCursesON = TRUE;
}
#endif /* defined (DJGPP) && !defined (USE_SLANG) */

PUBLIC void lynx_enable_mouse ARGS1(int,state)
{
   if (LYUseMouse == 0) 
     return;

#ifdef USE_SLANG_MOUSE
   SLtt_set_mouse_mode (state, 0);
   SLtt_flush_output ();
#else
#ifdef NCURSES_MOUSE_VERSION
     /* Inform ncurses that we're interested in knowing when mouse
      button 1 is clicked */
#ifndef _WINDOWS
   if (state)
     mousemask(BUTTON1_CLICKED, NULL);
   else
     mousemask(0, NULL);
#else
   if (state) mouse_set(BUTTON1_CLICKED && BUTTON2_CLICKED && BUTTON3_CLICKED);
#endif /* _WINDOWS */
#endif /* NCURSES_MOUSE_VERSION */

#if defined(DJGPP) && !defined(USE_SLANG)
     if (state)
       mouse_set(BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED);
#endif
#endif				       /* NOT USE_SLANG_MOUSE */
}

PUBLIC void stop_curses NOARGS
{
    echo();
#ifdef DJGPP
		  sock_exit();
#endif
#if defined (DOSPATH) && !defined (USE_SLANG)
	 clrscr();
/*
		  clear();
		  refresh();
*/
#else

    /*
     *	Fixed for better dumb terminal support.
     *	05-28-94 Lynx 2-3-1 Garrett Arch Blythe
     */
    if(LYCursesON == TRUE)	{
         lynx_enable_mouse (0);
         endwin();	/* stop curses */
    }

    fflush(stdout);
#endif /* DJGPP */
    fflush(stderr);

    LYCursesON = FALSE;

#ifndef VMS
#ifdef SIGTSTP
   if (!no_suspend)
       signal(SIGTSTP, SIG_DFL);
#endif /* SIGTSTP */
    signal(SIGINT, SIG_DFL);
#endif /* !VMS */
}


#ifdef VMS
/*
 *  Check terminal type, start curses & setup terminal.
 */
PUBLIC BOOLEAN setup ARGS1(
	char *,		terminal)
{
    int c;
    int status;
    char *dummy, *cp, term[81];
#ifdef USE_SLANG
    extern void longname();
#endif /* USE_SLANG */

    /*
     *  If the display was not set by a command line option then
     *  see if it is available from the environment.
     */
    if ((display = getenv(DISPLAY)) != NULL && *display == '\0')
        display = NULL;

    /*
     *  Get terminal type, and convert to lower case.
     */
    term[0] = '\0';
    longname(dummy, term);
    if (term[0] == '\0' && (form_get_data || form_post_data)) {
        /*
	 *  Some yoyo used these under conditions which require
	 *  -dump, so force that mode here. - FM
	 */
	dump_output_immediately = TRUE;
        LYcols = 80;
	keypad_mode = LINKS_ARE_NUMBERED;
	status = mainloop();
        (void) signal (SIGHUP, SIG_DFL);
        (void) signal (SIGTERM, SIG_DFL);
#ifdef SIGTSTP
	if (no_suspend)
	  (void) signal(SIGTSTP,SIG_DFL);
#endif /* SIGTSTP */
	exit(status);
    }
    for (cp=term; *cp!='\0'; cp++)
	if (isupper(*cp))
	    *cp = TOLOWER(*cp);

    printf("Terminal = %s\n", term);
    sleep(InfoSecs);
    if ((strlen(term) < 5) ||
        strncmp(term, "vt", 2) || !isdigit(term[2])) {
	printf(
	    "You must use a vt100, 200, etc. terminal with this program.\n");
	printf("Proceed (n/y)? ");
	c = getchar();
	if (c != 'y' && c != 'Y') {
	    printf("\n");
	    return(FALSE);
	}
	strcpy(term,"vt100");
    }

    ttopen();
    start_curses();

    LYlines = LINES;
    LYcols = COLS;

    return(TRUE);
}

#else	/* Not VMS: */

/*
 *  Check terminal type, start curses & setup terminal.
 */
PUBLIC BOOLEAN setup ARGS1(
	char *,		terminal)
{
    static char term_putenv[120];
    char buffer[120];

   /*
    *  If the display was not set by a command line option then
    *  see if it is available from the environment .
    */
    if ((display = getenv(DISPLAY)) != NULL && *display == '\0')
        display = NULL;

    if (terminal != NULL) {
	sprintf(term_putenv,"TERM=%s",terminal);
	(void) putenv(term_putenv);
    }

    /*
     *  Query the terminal type.
     */
    if (dumbterm(getenv("TERM"))) {
	char *s;

	printf("\n\n  Your Terminal type is unknown!\n\n");
	printf("  Enter a terminal type: [vt100] ");
	*buffer = '\0';
	fgets(buffer, sizeof(buffer), stdin);
	if ((s = strchr(buffer, '\n')) != NULL)
	    *s = '\0';

	if (strlen(buffer) == 0)
	    strcpy(buffer,"vt100"); 
	
	sprintf(term_putenv,"TERM=%s", buffer);
	putenv(term_putenv);
	printf("\nTERMINAL TYPE IS SET TO %s\n",getenv("TERM"));
	sleep(MESSAGESECS);
    }

    start_curses();

#if HAVE_TTYTYPE
    /*
     *  Get terminal type (strip 'dec-' from vms style types).
     */
    if (strncmp((CONST char*)ttytype, "dec-vt", 6) == 0) {
	(void) setterm(ttytype + 4);
    }
#endif /* HAVE_TTYTYPE */

    LYlines = LINES;
    LYcols = COLS;

    return(1);
}

PRIVATE int dumbterm ARGS1(
	char *,		terminal)
{
    int dumb = FALSE;

    /*
     *	Began checking for terminal == NULL in case that TERM environemnt
     *	variable is not set.  Thanks to Dick Wesseling (ftu@fi.ruu.nl).
     */
    if (terminal == NULL ||
	!strcasecomp(terminal, "network") ||
	!strcasecomp(terminal, "unknown") ||
	!strcasecomp(terminal, "dialup")  ||
	!strcasecomp(terminal, "dumb")    ||
	!strcasecomp(terminal, "switch")  ||
	!strcasecomp(terminal, "ethernet")  )
	dumb = TRUE;
    return(dumb);
}
#endif /* VMS */

#ifdef VMS
/*
 *	Cut-down termio --
 *		Do character-oriented stream input for Jeff.
 *		Code ripped off from Micro-Emacs 3.7 by Daniel Lawrence.
 *
 *		Ever-so-slightly modified by Kathryn Huxtable.  29-Jan-1991.
 *		Cut down for Lou.  8 Sep 1992.
 *		Cut down farther for Lou.  19 Apr 1993.
 *			We don't set PASSALL or PASTHRU since we don't
 *			want to block CTRL/C, CTRL/Y, CTRL/S or CTRL/Q.
 *			Simply setting NOECHO and doing timed reads
 *			is sufficient.
 *              Further mods by Fote.  29-June-1993
 *			ttopen() and ttclose() are now terminal initialization
 *			 and restoration procedures, called once at startup
 *			 and at exit, respectively, of the LYNX image.
 *			ttclose() should be called before an exit from LYNX
 *			 no matter how the exit is invoked.
 *			setup(terminal) does the ttopen().
 *			cleanup() calls cleanup_files() and ttclose().
 *			ttgetc() now handles NOECHO and NOFLITR (instead of
 *			 setting the terminal itself to NOECHO in ttopen()).
 *			VMSsignal() added for handling both Ctrl-C *and* Ctrl-Y
 *			 interrupts, and disabling system response to Ctrl-T.
 *              Further mods by Fote.  15-Dec-1993
 *			Added edit handler in ttopen() which will invoke
 *			 VMSexit() and behave intelligently on ACCVIO's.
 *              Further mods by Fote.  29-Dec-1993
 *			Simplified ttgetc().
 *		Further mods by Fote.  16-Jan-1994
 *			Added code in ttopen() which will invoke VMSVersion()
 *			 to get the version of VMS as VersionVMS for use by
 *			 by new or modified interrupt or spawning routines.
 *		Further mods by Fote.  27-Jan-1994
 *			Added back a typeahead() which supports 'z' or 'Z' as
 *			an "Zap transfer" command via HTCheckForInterrupt()
 *			in LYUtils.c.
 */

#include <descrip.h>
#include <iodef.h>
#include <ssdef.h>
#include <stdlib.h>
#include <msgdef.h>
#include <ttdef.h>
#include <tt2def.h>
#include <libclidef.h>
#include <lib$routines.h>
#include <starlet.h>
#include <clidef.h>
#include <syidef.h>
#ifdef signal
#undef signal
#endif /* signal */
#include <signal.h>
#ifdef system
#undef system
#endif /* system */
#include <processes.h>
#include "LYVMSdef.h"

#define EFN	0			/* Event flag			*/

static	unsigned char buffer[20];	/* Input buffer			*/
static	int	in_pos, in_len;		/* For escape sequences		*/
static	int	oldmode[3];		/* Old TTY mode bits		*/
static	int	newmode[3];		/* New TTY mode bits		*/
static	short	iochan;			/* TTY I/O channel		*/
static  $DESCRIPTOR(term_nam_dsc,"TT"); /* Descriptor for iochan	*/
static	unsigned long mask = LIB$M_CLI_CTRLY|LIB$M_CLI_CTRLT; /* ^Y and ^T */
static	unsigned long old_msk;		/* Saved control mask		*/
static	short	trap_flag = FALSE;	/* TRUE if AST is set		*/
BOOLEAN DidCleanup = FALSE;		/* Exit handler flag		*/
static char VersionVMS[20];		/* Version of VMS		*/

PUBLIC int VMSVersion ARGS2(
	char *,		VerString,
	int,		VerLen)
{
     unsigned long status, itm_cod = SYI$_VERSION;
     int i, verlen = 0;
     struct dsc$descriptor version;
     char *m;

     version.dsc$a_pointer = VerString;
     version.dsc$w_length = VerLen - 1;
     version.dsc$b_dtype = DSC$K_DTYPE_B;
     version.dsc$b_class = DSC$K_CLASS_S;

     status = lib$getsyi(&itm_cod, 0, &version, &verlen, 0, 0);
     if (!(status&1) || verlen == 0)
	  return 0;

     /*
      *  Cut out trailing spaces
      */
     for (m = VerString+verlen, i = verlen-1; i > 0 && VerString[i] == ' '; --i)
	  *(--m) = '\0';

     return strlen(VerString)+1;	/* Transmit ending 0 too */
}

PUBLIC void VMSexit NOARGS
{
    /*
     *  If we get here and DidCleanup is not set, it was via an
     *  ACCVIO, or outofmemory forced exit, so make *sure* we
     *  attempt a cleanup and reset the terminal.
     */
    if (!DidCleanup) {
        if (LYOutOfMemory == FALSE) {
            fprintf(stderr,
"\nA Fatal error has occured in %s Ver. %s\n", LYNX_NAME, LYNX_VERSION);
	    fprintf(stderr,
"\nPlease notify your system administrator to confirm a bug, and if\n");
	    fprintf(stderr,
"confirmed, to notify the lynx-dev list.  Bug reports should have concise\n");
	    fprintf(stderr,
"descriptions of the command and/or URL which causes the problem, the\n");
	    fprintf(stderr,
"operating system name with version number, the TCPIP implementation, the\n");
	    fprintf(stderr,
"TRACEBACK if it can be captured, and any other relevant information.\n");

	    if (LYTraceLogFP == NULL) {
		fprintf(stderr,"\nPress RETURN to clean up: ");
		(void) getchar();
	    }
	} else if (LYCursesON) {
	    _statusline(MEMORY_EXHAUSTED_ABORT);
	    sleep(AlertSecs);
	}
        cleanup();
    }
    if (LYOutOfMemory == TRUE) {
	printf("\r\n%s\r\n\r\n", MEMORY_EXHAUSTED_ABORT);
	fflush(stdout);
	fflush(stderr);
    }
}

/*
 *	TTOPEN --
 *		This function is called once to set up the terminal
 *		device streams.	 It translates TT until it finds
 *		the terminal, then assigns a channel to it, sets it
 *		to EDIT, and sets up the Ctrl-C and Ctrl-Y interrupt
 *		handling.
 */
PUBLIC int ttopen NOARGS
{
	extern	void cleanup_sig();
	int	iosb[2];
	int	status;
	static unsigned long condition;
	static struct _exit_block {
	    unsigned long forward;
	    unsigned long address;
	    unsigned long zero;
	    unsigned long condition;
	} exit_handler_block;

	status = sys$assign( &term_nam_dsc, &iochan, 0, 0 );
	if( status != SS$_NORMAL )
		exit( status );

	status = sys$qiow( EFN, iochan, IO$_SENSEMODE, &iosb, 0, 0,
			  &oldmode, sizeof(oldmode), 0, 0, 0, 0 );
	if( status != SS$_NORMAL )
		exit( status );

	status = iosb[0] & 0xFFFF;
	if( status != SS$_NORMAL )
		exit( status );

	newmode[0] = oldmode[0];
	newmode[1] = oldmode[1];
	newmode[2] = oldmode[2] | TT2$M_EDIT;

	status = sys$qiow( EFN, iochan, IO$_SETMODE, &iosb, 0, 0,
			  &newmode, sizeof(newmode), 0, 0, 0, 0 );
	if( status != SS$_NORMAL )
		exit( status );

	status = iosb[0] & 0xFFFF;
	if( status != SS$_NORMAL )
		exit( status );

	/*
	 *  Declare the exit handler block.
	 */
	exit_handler_block.forward   = 0;
	exit_handler_block.address   = (unsigned long) &VMSexit;
	exit_handler_block.zero      = 0;
	exit_handler_block.condition = (unsigned long) &condition;
	status = sys$dclexh(&exit_handler_block);
	if (status != SS$_NORMAL)
		exit( status );

	/*
	 *  Set the AST.
	 */
	lib$disable_ctrl(&mask, &old_msk);
	trap_flag = TRUE;
	status = sys$qiow ( EFN, iochan,
			    IO$_SETMODE|IO$M_CTRLCAST|IO$M_CTRLYAST,
			    &iosb, 0, 0,
			    &cleanup_sig, SIGINT, 0, 0, 0, 0 );
	if ( status != SS$_NORMAL ) {
		lib$enable_ctrl(&old_msk);
		exit ( status );
	}

	/*
	 *  Get the version of VMS.
	 */
	if (VMSVersion(VersionVMS, 20) < 3)
		/*
		 *  Load zeros on error.
		 */
		strcpy(VersionVMS, "V0.0-0");

	return(0);
}	/*  ttopen  */

/*
 *	TTCLOSE --
 *		This function gets called just before we go back home
 *		to the command interpreter.  It puts the terminal back
 *		in a reasonable state.
 */
PUBLIC int ttclose NOARGS
{
	int	status;
	int	iosb[1];

	status = sys$qiow( EFN, iochan, IO$_SETMODE, &iosb, 0, 0,
			  &oldmode, sizeof(oldmode), 0, 0, 0, 0 );

	if( status != SS$_NORMAL || (iosb[0] & 0xFFFF) != SS$_NORMAL )
		exit( status );

	if (trap_flag) {
	    status = sys$dassgn (iochan);
	    status = lib$enable_ctrl(&old_msk);
	    trap_flag = FALSE;
	}
	return(0);
}	/*  ttclose  */

/*
 *	TTGETC --
 *		Read a character from the terminal, with NOECHO and NOFILTR.
 */
PUBLIC int ttgetc NOARGS
{
    int status;
    unsigned short iosb[4];

    if (in_pos < in_len)
        return(buffer[in_pos++]);

    status = sys$qiow(EFN, iochan,
		      IO$_READVBLK|IO$M_NOECHO|IO$M_NOFILTR,
		      &iosb, 0, 0,
		      &buffer, 1, 0, 0, 0, 0);
    if ((status&1) == 1)
        status = iosb[0];
    if (status == SS$_PARTESCAPE) {
	/*
	 *  Escape sequence in progress.  Fake a successful read.
	 */
	status = 1;
    }
    if ((status&1) != 1 && status != SS$_DATAOVERUN)
        exit(status);
    in_pos = 1;
    in_len = iosb[1] + iosb[3];
    return(buffer[0]);
}

/*
 *	TYPEAHEAD -- Fote Macrides 27-Jan-1994
 *		Check whether a keystroke has been entered, and return
 *		 it, or -1 if none was entered.
 */
PUBLIC int typeahead NOARGS
{
    int status;
    unsigned short iosb[4];

    if (dump_output_immediately)
        return -1;

    if (in_pos < in_len)
        return(buffer[in_pos++]);

again:
    status = sys$qiow (EFN, iochan,
		       IO$_READVBLK|IO$M_TIMED|IO$M_NOECHO|IO$M_NOFILTR,
		       &iosb, 0, 0,
		       &buffer, 1, 0, 0, 0, 0);
    if ((status&1) == 1)
        status = iosb[0];
    if (status == SS$_PARTESCAPE) {
	/*
	 *  Escape sequence in progress, finish reading it.
	 */
	goto again;
    }

    in_pos = 1;
    in_len = iosb[1] + iosb[3];
    if (status == SS$_TIMEOUT || status == SS$_DATAOVERUN)
        return(-1);
    return (buffer[0]);
}

/*
 *	VMSSIGNAL -- Fote Macrides 29-Jun-1993
 *		Sets up AST for both Ctrl-C and Ctrl-Y, with system response
 *		 to Ctrl-T disabled.  If called with a sig other than SIGINT,
 *		 it will use the C library's system(sig, func).
 *              The equivalent of VMSsignal(SIGINT, cleanup_sig) is done on
 *               intialization by ttopen(), so don't do it again.
 *		VMSsignal(SIGINT, SIG_DFL) is treated as a call to ttclose().
 *              Call VMSsignal(SIGINT, SIG_IGN) before system() calls to
 *               enable Ctrl-C and Ctrl-Y in the subprocess, and then call
 *               VMSsignal(SIG_INT, cleanup_sig) on return from the subprocess.
 *		For func's which set flags and do not invoke an exit from
 *		 LYNX, the func should reassert itself.
 *              The VMS signal() calls do not fully emulate the Unix calls,
 *		 and VMSsignal() is just a "helper", also not a full emulation.
 */

PUBLIC void *VMSsignal (sig,func)
int sig;
void (*func)();
{
	int status;
	short iosb[4];
	static int SIG_IGN_flag;

	/*
	 *  Pass all signals other than SIGINT to signal().
	 *  Also pass SIGINT to signal() if we're dumping.
	 */
	if (sig != SIGINT || dump_output_immediately) {
	    signal(sig, func);
	    return;
	}

	/*
	 *  If func is SIG_DFL, treat it as ttclose().
	 */
	if (func == SIG_DFL) {
	    ttclose();
	    return;
	}

	/*
	 *  Clear any previous AST.
	 */
	if (trap_flag) {
	    status = sys$dassgn (iochan);
	    status = lib$enable_ctrl(&old_msk);
	    trap_flag = FALSE;
	}

	/*
	 *  If func is SIG_IGN, leave the TT channel closed and the
	 *  system response to interrupts enabled for system() calls.
	 */
	if (func == SIG_IGN)
	    return;

	/*
	 *  If we get to here, we have a LYNX func, so set the AST.
	 */
	lib$disable_ctrl(&mask, &old_msk);
	trap_flag = TRUE;
	status = sys$assign (&term_nam_dsc, &iochan, 0, 0 );
	status = sys$qiow ( EFN, iochan,
			    IO$_SETMODE|IO$M_CTRLCAST|IO$M_CTRLYAST,
			    &iosb, 0, 0,
			    func, SIGINT, 0, 0, 0, 0 );

}	/* VMSsignal */

/*
 *  DCLspawn_exception, spawn_DCLprocess, DCLsystem -- F.Macrides 16-Jan-1994
 *	Exception-handler routines for regulating interrupts and enabling
 *	Control-T during spawns.  Includes TRUSTED flag for versions of VMS
 *	which require it in captive accounts.  This code should be used
 *	instead of the VAXC or DECC system(), by including LYSystem.h in
 *	modules which have system() calls.  It helps ensure that we return
 *	to Lynx instead of breaking out to DCL if a user issues interrupts
 *	or generates an ACCVIO during spawns.
 */
#ifdef __DECC
PRIVATE unsigned int DCLspawn_exception ARGS2(
	void *,		sigarr,
	void *,		mecharr)
{
#else
PRIVATE int DCLspawn_exception ARGS2(
	void *,		sigarr,
	void *,		mecharr)
{
#endif /* __DECC */
     int status;
     
     status = lib$sig_to_ret(sigarr, mecharr);
     return(SS$_UNWIND);
}

PRIVATE int spawn_DCLprocess ARGS1(
	char *,		command)
{
     int status;
     unsigned long Status = 0;
     /*
      *  Keep DECC from complaining.
      */
     struct dsc$descriptor_s  command_desc;
     command_desc.dsc$w_length  = strlen(command);
     command_desc.dsc$b_class   = DSC$K_CLASS_S;
     command_desc.dsc$b_dtype   = DSC$K_DTYPE_T;
     command_desc.dsc$a_pointer = command;

     VAXC$ESTABLISH(DCLspawn_exception);

#ifdef __ALPHA /** OpenVMS/AXP lacked the TRUSTED flag before v6.1 **/
     if (VersionVMS[1] > '6' ||
         (VersionVMS[1] == '6' && VersionVMS[2] == '.' &&
	  VersionVMS[3] >= '1')) {
#else
     if (VersionVMS[1] >= '6') {
#endif /* __ALPHA */
	 /*
	  *  Include TRUSTED flag.
	  */
	 unsigned long trusted = CLI$M_TRUSTED;
         status = lib$spawn(&command_desc,0,0,&trusted,
	 		    0,0,&Status);
	 /*
	  *  If it was invalid, try again without the flag.
	  */
         if (status == LIB$_INVARG)
            status = lib$spawn(&command_desc,0,0,0,
	    		       0,0,&Status );
     } else
	 status = lib$spawn(&command_desc,0,0,0,
	 		    0,0,&Status);
     /*
      *  Return -1 on error.
      */
     if ((status&1) != 1 || (Status&1) != 1)
         return(-1);
     /*
      *  Return 0 on success.
      */
     return(0);
}

PUBLIC int DCLsystem ARGS1(
	char *,		command)
{
     int status;
     extern void controlc();
     
     VMSsignal(SIGINT, SIG_IGN);
     status = spawn_DCLprocess(command);
     VMSsignal(SIGINT, cleanup_sig);
     /*
      *  Returns 0 on success, -1 any error.
      */
     return(status);
}

#ifndef USE_SLANG
/*
**  This function boxes windows with graphic characters for curses.
**  Pass it the window, it's height, and it's width. - FM
*/
PUBLIC void VMSbox ARGS3(
	WINDOW *,	win,
	int,		height,
	int,		width)
{
    int i;

    wmove(win, 0, 0);
    waddstr(win, "\033)0\016l");
    for (i = 1; i < width; i++)
       waddch(win, 'q');
    waddch(win, 'k');
    for (i = 1; i < height-1; i++) {
        wmove(win, i, 0);
	waddch(win, 'x');
        wmove(win, i, width-1);
	waddch(win, 'x');
    }
    wmove(win, i, 0);
    waddch(win, 'm');
    for (i = 1; i < width; i++)
       waddch(win, 'q');
    waddstr(win, "j\017");
}
#endif /* !USE_SLANG */
#endif /* VMS */



PUBLIC void lynx_start_underline_color NOARGS
{
   start_underline ();
}

PUBLIC void lynx_stop_underline_color NOARGS
{
   stop_underline ();
}

PUBLIC void lynx_start_bold_color NOARGS
{
   start_bold ();
}

PUBLIC void lynx_stop_bold_color NOARGS
{
   stop_bold ();
}

PUBLIC void lynx_start_title_color NOARGS
{
}

PUBLIC void lynx_stop_title_color NOARGS
{
}

PUBLIC void lynx_start_link_color ARGS1(int, flag)
{
    if (flag) {
	/* makes some terminals work wrong because
	 * they can't handle two attributes at the 
	 * same time
	 */
	/* start_bold();  */
	start_reverse();
#if defined(USE_SLANG) || defined(FANCY_CURSES)
	start_underline ();
#endif /* USE_SLANG */
     } else {
	start_bold();
     }
}

PUBLIC void lynx_stop_link_color ARGS1(int, flag)
{
#ifdef USE_COLOR_STYLE
    LynxChangeStyle(flag == ON ? s_alink : s_a, ABS_OFF, 0);
#else
    if (flag) {
	stop_reverse();
#if defined(USE_SLANG) || defined(FANCY_CURSES)
	stop_underline ();
#endif /* USE_SLANG */
    } else
	stop_bold();
#endif
}


PUBLIC void lynx_stop_target_color NOARGS
{
   stop_underline();
   stop_reverse();
   stop_bold();
}

PUBLIC void lynx_start_target_color NOARGS
{
   start_bold();
   start_reverse();
   start_underline();
}


PUBLIC void lynx_start_status_color NOARGS
{
#if USE_COLOR_TABLE && defined(COLOR_CURSES)
   lynx_set_color (2);
#else
   start_reverse ();
#endif
}

PUBLIC void lynx_stop_status_color NOARGS
{
#if USE_COLOR_TABLE && defined(COLOR_CURSES)
   lynx_set_color (0);
#else
   stop_reverse ();
#endif
}

PUBLIC void lynx_start_h1_color NOARGS
{
   if (bold_H1 || bold_headers)
     start_bold();
}

PUBLIC void lynx_stop_h1_color NOARGS
{
   if (bold_H1 || bold_headers)
     stop_bold();
}

PUBLIC void lynx_start_prompt_color NOARGS
{
   start_reverse ();
}

PUBLIC void lynx_stop_prompt_color NOARGS
{
   stop_reverse ();
}

PUBLIC void lynx_start_radio_color NOARGS
{
   start_bold ();
}

PUBLIC void lynx_stop_radio_color NOARGS
{
   stop_bold ();
}

PUBLIC void lynx_stop_all_colors NOARGS
{
   stop_underline ();
   stop_reverse ();
   stop_bold ();
}
   
	
#include "HTUtils.h"
#include "tcp.h"
#include "HTFTP.h"
#include "HTML.h"
#include "LYCurses.h"
#include "LYUtils.h"
#include "LYStrings.h"
#include "LYGlobalDefs.h"
#include "LYOptions.h"
#include "LYSignal.h"
#include "LYClean.h"
#include "LYCharSets.h"
#include "LYCharUtils.h"
#include "LYKeymap.h"
#include "LYrcFile.h"
#include "HTAlert.h"
#include "LYBookmark.h"

#include "LYLeaks.h"

#define FREE(x) if (x) {free(x); x = NULL;}

#ifdef VMS
#define DISPLAY "DECW$DISPLAY"
#else
#define DISPLAY "DISPLAY"
#endif /* VMS */

#define COL_OPTION_VALUES 36  /* display column where option values start */

BOOLEAN term_options;
PRIVATE void terminate_options  PARAMS((int sig));
PRIVATE int boolean_choice PARAMS((int status, int line,
				   int column, char **choices));

#define MAXCHOICES 10

PRIVATE void option_statusline ARGS1(char *,text)
{
    char buffer[256];

    if (!text || text == NULL)
        return;

        /* don't print statusline messages if dumping to stdout
         */
    if (dump_output_immediately)
        return;

    /* make sure text is not longer than COLS */
    LYstrncpy(buffer, text, LYcols-1);

    move(LYlines-1, 0);

    clrtoeol();
    if (text != NULL && *buffer) {
        start_reverse();
        addstr(buffer);
        stop_reverse();
    }

    refresh();
}

PUBLIC void options NOARGS
{
#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
    int itmp;
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */
    int response, ch;
    /* if the user changes the display I need memory to put it in */
    char display_option[256]; 
#ifndef VMS
    static char putenv_command[142];
#endif /* !VMS */
    char *choices[MAXCHOICES];
    int CurrentCharSet = current_char_set;
    BOOLEAN CurrentRawMode = LYRawMode;
    
#ifdef DIRED_SUPPORT
#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
    if (LYlines < 24) {
        HTAlert(OPTION_SCREEN_NEEDS_24);
	return;
    }
#else
    if (LYlines < 23) {
        HTAlert(OPTION_SCREEN_NEEDS_23);
	return;
    }
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */
#else
#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
    if (LYlines < 23) {
        HTAlert(
	"Screen height must be at least 23 lines for the Options menu!");
	return;
    }
#else
    if (LYlines < 22) {
        HTAlert(OPTION_SCREEN_NEEDS_22);
	return;
    }
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */
#endif /* DIRED_SUPPORT */

    term_options = FALSE;
    signal(SIGINT, terminate_options);

draw_options:
    response = 0;
    clear(); 
    move(0, 5);
    if (bold_H1 || bold_headers)
        start_bold();
    printw("         Options Menu (%s Version %s)",
    	   			   LYNX_NAME, LYNX_VERSION);
    if (bold_H1 || bold_headers)
        stop_bold();
    move(L_EDITOR, 5);  
    printw("E)ditor                      : %s",
    		((editor && *editor) ? editor : "NONE"));

    move(L_DISPLAY, 5);  
    printw("D)ISPLAY variable            : %s",
    		((display && *display) ? display : "NONE"));

    move(L_MAIL_ADDRESS, 5);  
    printw("P)ersonal mail address       : %s",
    		((personal_mail_address && *personal_mail_address) ?
					     personal_mail_address : "NONE"));

    move(L_HOME, 5);
    printw("mu(L)ti-bookmarks: %s",
		(LYMultiBookmarks ? (LYMBMAdvanced ?
                                     "ADVANCED" : "STANDARD") : "OFF"));
    move(L_HOME, B_BOOK);
    if (LYMultiBookmarks) {
        printw("review/edit B)ookmarks files");
    } else {
        printw("B)ookmark file: %s",
    		((bookmark_page && *bookmark_page) ? bookmark_page : "NONE"));
    }

    move(L_FTPSTYPE, 5);
    printw("F)TP sort criteria           : %s",(HTfileSortMethod==FILE_BY_NAME ?
					"By Filename" :
					  (HTfileSortMethod==FILE_BY_SIZE ?
					    "By Size" : 
					      (HTfileSortMethod==FILE_BY_TYPE ?
						"By Type" : "By Date"))));
    move(L_SSEARCH, 5); 
    printw("S)earching type              : %s",(case_sensitive ?
				        "CASE SENSITIVE" : "CASE INSENSITIVE"));

    move(L_CHARSET, 5);
    printw("display (C)haracter set      : %s", 
    					LYchar_set_names[current_char_set]);
    
    move(L_RAWMODE, 5);
    printw("Raw 8-bit or CJK m(O)de      : %s", (LYRawMode ? "ON" : "OFF"));

    move(L_LANGUAGE, 5);
    printw("preferred document lan(G)uage: %s",
    		((language && *language) ? language : "NONE"));

    move(L_PREF_CHARSET, 5);
    printw("preferred document c(H)arset : %s",
    		((pref_charset && *pref_charset) ? pref_charset : "NONE"));

    move(L_BOOL_A, B_VIKEYS);
    printw("V)I keys: %s", (vi_keys ? "ON" : "OFF"));
    
    move(L_BOOL_A, B_EMACSKEYS);
    printw("e(M)acs keys: %s", (emacs_keys ? "ON" : "OFF"));
    
    move(L_BOOL_A, B_SHOW_DOTFILES);
    printw("sho(W) dot files: %s",
    			((!no_dotfiles && show_dotfiles) ? "ON" : "OFF"));

    move(L_SELECT_POPUPS, 5);
    printw("popups for selec(T) fields   : %s",
    			(LYSelectPopups ? "ON" : "OFF"));

    move(L_KEYPAD, 5); 
    printw("K)eypad mode                 : %s", 
			   		  (keypad_mode == NUMBERS_AS_ARROWS ? 
					   "Numbers act as arrows" : 
				           "Links are numbered"));

    move(L_LINEED, 5);
    printw("li(N)e edit style            : %s",
    					   LYLineeditNames[current_lineedit]);

#ifdef DIRED_SUPPORT
    move(L_DIRED, 5);
    printw("l(I)st directory style       : %s",
                     (dir_list_style == FILES_FIRST ? "Files first          " :
		     (dir_list_style == MIXED_STYLE ? "Mixed style          " : 
                                                      "Directories first    ")));
#endif /* DIRED_SUPPORT */

    move(L_USER_MODE, 5);
    printw("U)ser mode                   : %s",
			(user_mode == NOVICE_MODE ? "Novice" : 
			(user_mode == INTERMEDIATE_MODE ? "Intermediate" :
							     "Advanced")));

    move(L_USER_AGENT, 5);
    printw("user (A)gent                 : %s",
    		((LYUserAgent && *LYUserAgent) ? LYUserAgent : "NONE"));


#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
    move(L_EXEC, 5);
    printw("local e(X)ecution links      : ");
#ifndef NEVER_ALLOW_REMOTE_EXEC
    addstr((local_exec ? "ALWAYS ON" :
                    (local_exec_on_local_files ? "FOR LOCAL FILES ONLY" :
                                                              "ALWAYS OFF")));
#else
    addstr(local_exec_on_local_files ? "FOR LOCAL FILES ONLY" :
                                                              "ALWAYS OFF");
#endif /* NEVER_ALLOW_REMOTE_EXEC */
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */

    move(LYlines-3, 2);
    addstr(SELECT_SEGMENT);
    standout();
    addstr(CAP_LETT_SEGMENT);
    standend();
    addstr(OF_OPT_LINE_SEGMENT);
    if (!no_option_save) {
        addstr(" '");
	standout();
	addstr(">");
	standend();
	addstr("'");
	addstr(TO_SAVE_SEGMENT);
    }
    addstr(OR_SEGMENT);
    addstr("'");
    standout();
    addstr("r");
    standend();
    addstr("'");
    addstr(TO_RETURN_SEGMENT);

    while (TOUPPER(response) != 'R' &&
    	   !LYisNonAlnumKeyname(response, LYK_PREV_DOC) &&
           response != '>' && !term_options &&
	   response != 7 &&  response != 3) {

           move(LYlines-2, 0);
	   start_reverse();
	   addstr(COMMAND_PROMPT);
	   stop_reverse();

	   refresh();
           response = LYgetch();
	   if (term_options || response == 7 || response == 3)
	       response = 'R';
	   if (LYisNonAlnumKeyname(response, LYK_REFRESH)) {
	       clearok(curscr, TRUE);
	       goto draw_options;
	   }
	   switch (response) {
	 	case 'e':  /* change the editor */
		case 'E':
	                if (no_editor) {
			    option_statusline(EDIT_DISABLED);
	                } else if (system_editor ) {
			    option_statusline(EDITOR_LOCKED);
			} else {
			    if (editor && *editor)
			        strcpy(display_option, editor);
			    else {  /* clear the NONE */
				move(L_EDITOR, COL_OPTION_VALUES);
				addstr("    ");
			        *display_option = '\0';
			    }
			    option_statusline(ACCEPT_DATA);
			    move(L_EDITOR, COL_OPTION_VALUES);  
			    standout();
			    ch = LYgetstr(display_option, VISIBLE,
			    		  sizeof(display_option), NORECALL);
			    standend();
			    move(L_EDITOR, COL_OPTION_VALUES);
			    if (term_options || ch == -1) {
			        addstr((editor && *editor) ?
						    editor : "NONE");
			    } else if (*display_option == '\0') {
				FREE(editor);
				addstr("NONE");
			    } else {
			        StrAllocCopy(editor, display_option);
				addstr(display_option);
			    }
			    clrtoeol();
			    option_statusline(VALUE_ACCEPTED);
			}
			response = ' ';
			break;

		case 'd':  /* change the display */
		case 'D':
			if (display && *display) {
			    strcpy(display_option, display);
			} else {  /* clear the NONE */
			    move(L_DISPLAY, COL_OPTION_VALUES);
			    addstr("    ");
			    *display_option = '\0';
			}
			option_statusline(ACCEPT_DATA);
	                move(L_DISPLAY, COL_OPTION_VALUES);
			standout();
			ch = LYgetstr(display_option, VISIBLE,
				      sizeof(display_option), NORECALL);
			standend();
	                move(L_DISPLAY, COL_OPTION_VALUES);
			if ((term_options || ch == -1) ||
			    (display != NULL &&
#ifdef VMS
			     0 == strcasecomp(display, display_option)))
#else
			     0 == strcmp(display, display_option)))
#endif /* VMS */
			{
			    /*
			     *  Cancelled, or a non-NULL display string
			     *  wasn't changed. - FM
			     */
			    addstr((display && *display) ? display : "NONE");
			    clrtoeol();
			    option_statusline(VALUE_ACCEPTED);
			    response = ' ';
			    break;
			} else if (*display_option == '\0') {
			    if ((display == NULL) ||
			        (display != NULL && *display == '\0')) {
				/*
				 *  NULL or zero-length display string
				 *  wasn't changed. - FM
				 */
			        addstr("NONE");
				clrtoeol();
				option_statusline(VALUE_ACCEPTED);
				response = ' ';
				break;
			    }
			}
			/*
			 *  Set the new DISPLAY variable. - FM
			 */
#ifdef VMS
			{
			    int i;
			    for (i = 0; display_option[i]; i++)
			        display_option[i] = TOUPPER(display_option[i]);
			    Define_VMSLogical(DISPLAY, display_option);
			}
#else
			sprintf(putenv_command, "DISPLAY=%s", display_option);
			putenv(putenv_command);
#endif /* VMS */
			if ((display = getenv(DISPLAY)) != NULL &&
			    *display == '\0') {
			    display = NULL;
			}
			addstr(display ? display : "NONE");
			clrtoeol();
			if ((display == NULL && *display_option == '\0') ||
			    (display != NULL &&
			     0 == strcmp(display, display_option))) {
			    if (display == NULL &&
			        LYisConfiguredForX == TRUE) {
				option_statusline(VALUE_ACCEPTED_WARNING_X);
			    } else if (display != NULL &&
			        LYisConfiguredForX == FALSE) {
				option_statusline(VALUE_ACCEPTED_WARNING_NONX);
			    } else {
			        option_statusline(VALUE_ACCEPTED);
			    }
			} else {
			    if (*display_option) {
			        option_statusline(FAILED_TO_SET_DISPLAY);
			    } else {
			        option_statusline(FAILED_CLEAR_SET_DISPLAY);
			    }
			}
			response = ' ';
			break;

		case 'l':
		case 'L':
			if (LYMBMBlocked) {
			    option_statusline(MULTIBOOKMARKS_DISALLOWED);
			    response = ' ';
			    break;
			}
			choices[0] = NULL;
			StrAllocCopy(choices[0],"OFF     ");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"STANDARD");
			choices[2] = NULL;
			StrAllocCopy(choices[2],"ADVANCED");
			choices[3] = NULL;
			LYMultiBookmarks = boolean_choice(LYMultiBookmarks *
                                                          (1 + LYMBMAdvanced),
                                                          L_HOME, C_MULTI,
                                                          choices);
			FREE(choices[0]);
			FREE(choices[1]);
			FREE(choices[2]);
                        if (LYMultiBookmarks == 2) {
                            LYMultiBookmarks = TRUE;
                            LYMBMAdvanced = TRUE;
                        } else {
                            LYMBMAdvanced = FALSE;
                        }
			
			move(L_HOME, B_BOOK);
			clrtoeol();
    			if (LYMultiBookmarks) {
    			    printw("review/edit B)ookmarks files");
    			} else {
			    printw("B)ookmark file: %s",
    		((bookmark_page && *bookmark_page) ? bookmark_page : "NONE"));
    			}
			response = ' ';
			break;

		case 'b':  /* change the bookmark page location */
		case 'B':
			/* anonymous users should not be allowed to
			 * change the bookmark page
			 */
			if (!no_bookmark) {
			    if (LYMultiBookmarks) {
				edit_bookmarks();
				signal(SIGINT, terminate_options);
				goto draw_options;
			    }
			    if (bookmark_page && *bookmark_page)
			        strcpy(display_option, bookmark_page);
			    else {  /* clear the NONE */
				move(L_HOME, C_DEFAULT);
				clrtoeol();
			        *display_option = '\0';
			    }
			    option_statusline(ACCEPT_DATA);
			    move(L_HOME, C_DEFAULT);  
			    standout();
			    ch = LYgetstr(display_option, VISIBLE,
			    		  sizeof(display_option), NORECALL);
			    standend();
			    move(L_HOME, C_DEFAULT);
			    if (term_options ||
			        ch == -1 || *display_option == '\0') {
			        addstr((bookmark_page && *bookmark_page) ?
						    bookmark_page : "NONE");
			    } else {
			        StrAllocCopy(bookmark_page, display_option);
				StrAllocCopy(MBM_A_subbookmark[0],
					     bookmark_page);
				addstr(display_option);
			    }
			    clrtoeol();
			    option_statusline(VALUE_ACCEPTED);
			} else { /* anonymous */
			    option_statusline(BOOKMARK_CHANGE_DISALLOWED);
			}
			response = ' ';
			break;

		case 'f':
		case 'F':
                        /* copy strings into choice array */
                        choices[0] = NULL;
                        StrAllocCopy(choices[0],"By Filename");
                        choices[1] = NULL;
                        StrAllocCopy(choices[1],"By Type    ");
                        choices[2] = NULL;
                        StrAllocCopy(choices[2],"By Size    ");
                        choices[3] = NULL;
                        StrAllocCopy(choices[3],"By Date    ");
                        choices[4] = NULL;
                        HTfileSortMethod = boolean_choice(HTfileSortMethod,
                                               L_FTPSTYPE, -1, choices);
                        FREE(choices[0]);
                        FREE(choices[1]);
                        FREE(choices[2]);
			response = ' ';
                        break;

		case 'p':  /* change personal mail address for From headers */
		case 'P':
			if (personal_mail_address && *personal_mail_address)
			    strcpy(display_option, personal_mail_address);
			else {  /* clear the NONE */
			    move(L_MAIL_ADDRESS, COL_OPTION_VALUES);
			    addstr("    ");
			    *display_option = '\0';
			}
			option_statusline(ACCEPT_DATA);
	                move(L_MAIL_ADDRESS, COL_OPTION_VALUES);
			standout();
			ch = LYgetstr(display_option, VISIBLE,
				      sizeof(display_option), NORECALL);
			standend();
	                move(L_MAIL_ADDRESS, COL_OPTION_VALUES);
			if (term_options || ch == -1) {
			    addstr((personal_mail_address &&
				    *personal_mail_address) ?
				      personal_mail_address : "NONE");
			} else if (*display_option == '\0') {
			    FREE(personal_mail_address);
			    addstr("NONE");
			} else {
			    StrAllocCopy(personal_mail_address, display_option);
			    addstr(display_option);
			}
			clrtoeol();
			option_statusline(VALUE_ACCEPTED);
			response = ' ';
			break;

		case 's':
		case 'S':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"CASE INSENSITIVE");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"CASE SENSITIVE  ");
			choices[2] = NULL;
			case_sensitive = boolean_choice(case_sensitive,
						L_SSEARCH, -1, choices);
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'c':
		case 'C':
			current_char_set = boolean_choice(current_char_set,
			    		L_CHARSET, -1, LYchar_set_names);
			/*
			 *  Set the raw 8-bit or CJK mode defaults and
			 *  character set if changed. - FM
			 */
			if (CurrentCharSet != current_char_set) {
			    HTMLSetRawModeDefault(current_char_set);
			    LYUseDefaultRawMode = TRUE;
			    HTMLUseCharacterSet(current_char_set);
			    CurrentCharSet = current_char_set;
			    CurrentRawMode = LYRawMode;
			    move(L_RAWMODE, COL_OPTION_VALUES);
			    clrtoeol();
			    addstr(LYRawMode ? "ON " : "OFF");
			}
			response = ' ';
			break;

		case 'o':
		case 'O':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0], "OFF");
			choices[1] = NULL;
			StrAllocCopy(choices[1], "ON ");
			choices[2] = NULL;
			LYRawMode = boolean_choice(LYRawMode,
						   L_RAWMODE, -1, choices);
			/*
			 *  Set the LYUseDefaultRawMode value and character
			 *  handling if LYRawMode was changed. - FM
			 */
			if (CurrentRawMode != LYRawMode) {
			    HTMLSetUseDefaultRawMode(current_char_set,
			    			     LYRawMode);
			    HTMLSetCharacterHandling(current_char_set);
			    CurrentRawMode = LYRawMode;
			}
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'g':  /* change language preference */
		case 'G':
			if (language && *language)
			    strcpy(display_option, language);
			else {  /* clear the NONE */
			    move(L_LANGUAGE, COL_OPTION_VALUES);
			    addstr("    ");
			    *display_option = '\0';
			}
			option_statusline(ACCEPT_DATA);
			move(L_LANGUAGE, COL_OPTION_VALUES);
			standout();
			ch = LYgetstr(display_option, VISIBLE,
				      sizeof(display_option), NORECALL);
			standend();
			move(L_LANGUAGE, COL_OPTION_VALUES);
			if (term_options || ch == -1) {
			    addstr((language && *language) ?
			    	   language : "NONE");
			} else if (*display_option == '\0') {
			    FREE(language);
			    addstr("NONE");
			} else {
			    StrAllocCopy(language, display_option);
			    addstr(display_option);
			}
			clrtoeol();
			option_statusline(VALUE_ACCEPTED);
			response = ' ';
			break;

		case 'h':  /* change character set preference */
		case 'H':
			if (pref_charset && *pref_charset)
			    strcpy(display_option, pref_charset);
			else {  /* clear the NONE */
			    move(L_PREF_CHARSET, COL_OPTION_VALUES);
			    addstr("    ");
			    *display_option = '\0';
			}
			option_statusline(ACCEPT_DATA);
			move(L_PREF_CHARSET, COL_OPTION_VALUES);
			standout();
			ch = LYgetstr(display_option, VISIBLE,
				      sizeof(display_option), NORECALL);
			standend();
			move(L_PREF_CHARSET, COL_OPTION_VALUES);
			if (term_options || ch == -1) {
			    addstr((pref_charset && *pref_charset) ?
			    	   pref_charset : "NONE");
			} else if (*display_option == '\0') {
			    FREE(pref_charset);
			    addstr("NONE");
			} else {
			    StrAllocCopy(pref_charset, display_option);
			    addstr(display_option);
			}
			clrtoeol();
			option_statusline(VALUE_ACCEPTED);
			response = ' ';
			break;

		case 'v':
		case 'V':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"OFF");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"ON ");
			choices[2] = NULL;
			vi_keys = boolean_choice(vi_keys,
						 L_BOOL_A, C_VIKEYS,
						 choices);
			if (vi_keys)
                            set_vi_keys();
                        else
                            reset_vi_keys();
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'M':
		case 'm':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"OFF");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"ON ");
			choices[2] = NULL;
			emacs_keys = boolean_choice(emacs_keys,
						    L_BOOL_A, C_EMACSKEYS,
						    choices);
                        if (emacs_keys)
                            set_emacs_keys();
                        else
                            reset_emacs_keys();
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'W':
		case 'w':
			   if (no_dotfiles) {
			       option_statusline(DOTFILE_ACCESS_DISABLED);
			   } else {
			       /* copy strings into choice array */
			       choices[0] = NULL;
			       StrAllocCopy(choices[0],"OFF");
			       choices[1] = NULL;
			       StrAllocCopy(choices[1],"ON ");
			       choices[2] = NULL;
			       show_dotfiles = boolean_choice(show_dotfiles,
							      L_BOOL_A,
							      C_SHOW_DOTFILES, 
							      choices);
			       FREE(choices[0]);
			       FREE(choices[1]);
			   }
			   response = ' ';
			   break;

		case 't':
		case 'T':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0], "OFF");
			choices[1] = NULL;
			StrAllocCopy(choices[1], "ON ");
			choices[2] = NULL;
			LYSelectPopups = boolean_choice(LYSelectPopups,
							L_SELECT_POPUPS, -1,
							choices);
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'k':
		case 'K':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"Numbers act as arrows");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"Links are numbered   ");
			choices[2] = NULL;
			keypad_mode = boolean_choice(keypad_mode,
			       			     L_KEYPAD, -1, choices);
                        if (keypad_mode == NUMBERS_AS_ARROWS)
                            set_numbers_as_arrows();
                        else
                            reset_numbers_as_arrows();
			FREE(choices[0]);
			FREE(choices[1]);
			response = ' ';
			break;

		case 'n':
		case 'N':
			current_lineedit = boolean_choice(current_lineedit,
			    		L_LINEED, -1, LYLineeditNames);
			response = ' ';
			break;

#ifdef DIRED_SUPPORT
		case 'i':
		case 'I':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"Directories first");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"Files first      ");
			choices[2] = NULL;
			StrAllocCopy(choices[2],"Mixed style      ");
			choices[3] = NULL;
			dir_list_style = boolean_choice(dir_list_style,
							L_DIRED, -1, choices);
			FREE(choices[0]);
			FREE(choices[1]);
			FREE(choices[2]);
			response = ' ';
			break;
#endif /* DIRED_SUPPORT */

		case 'u':
		case 'U':
			/* copy strings into choice array */
			choices[0] = NULL;
			StrAllocCopy(choices[0],"Novice      ");
			choices[1] = NULL;
			StrAllocCopy(choices[1],"Intermediate");
			choices[2] = NULL;
			StrAllocCopy(choices[2],"Advanced    ");
			choices[3] = NULL;
			user_mode = boolean_choice(user_mode,
						   L_USER_MODE, -1, choices);
			FREE(choices[0]);
			FREE(choices[1]);
			FREE(choices[2]);
			if(user_mode == NOVICE_MODE)
			   display_lines = LYlines-4;
			else
			   display_lines = LYlines-2;
			response = ' ';
			break;

		case 'a':
		case 'A':
			if (!no_useragent) {
			    if (LYUserAgent && *LYUserAgent)
			        strcpy(display_option, LYUserAgent);
			    else {  /* clear the NONE */
				move(L_HOME, COL_OPTION_VALUES);
				addstr("    ");
			        *display_option = '\0';
			    }
			    option_statusline(ACCEPT_DATA_OR_DEFAULT);
			    move(L_USER_AGENT, COL_OPTION_VALUES);  
			    standout();
			    ch = LYgetstr(display_option, VISIBLE,
			    		  sizeof(display_option), NORECALL);
			    standend();
			    move(L_USER_AGENT, COL_OPTION_VALUES);
			    if (term_options || ch == -1) {
			        addstr((LYUserAgent &&
					*LYUserAgent) ?
					  LYUserAgent : "NONE");
			    } else if (*display_option == '\0') {
			        StrAllocCopy(LYUserAgent, LYUserAgentDefault);
				addstr((LYUserAgent &&
					*LYUserAgent) ?
					  LYUserAgent : "NONE");
			    } else {
			        StrAllocCopy(LYUserAgent, display_option);
				addstr(display_option);
			    }
			    clrtoeol();
			    if (LYUserAgent && *LYUserAgent &&
			    	!strstr(LYUserAgent, "Lynx") &&
				!strstr(LYUserAgent, "lynx")) {
				option_statusline(UA_COPYRIGHT_WARNING);
			    } else {
			        option_statusline(VALUE_ACCEPTED);
			    }
			} else { /* disallowed */
			    option_statusline(UA_COPYRIGHT_WARNING);
			}
			response = ' ';
			break;

#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
		case 'x':  /* local exec */
		case 'X':
			if (!exec_frozen) {
#ifndef NEVER_ALLOW_REMOTE_EXEC
			    if (local_exec) {
				itmp = 2;
			    } else {
#else
			  {
#endif /* NEVER_ALLOW_REMOTE_EXEC */
			  	if (local_exec_on_local_files)
				    itmp= 1;
				else
				    itmp = 0;
			   }
			   /* copy strings into choice array */
			   choices[0] = NULL;
			   StrAllocCopy(choices[0],"ALWAYS OFF          ");
			   choices[1] = NULL;
			   StrAllocCopy(choices[1],"FOR LOCAL FILES ONLY");
			   choices[2] = NULL;
#ifndef NEVER_ALLOW_REMOTE_EXEC
			   StrAllocCopy(choices[2],"ALWAYS ON           ");
			   choices[3] = NULL;
#endif /* NEVER_ALLOW_REMOTE_EXEC */
			   itmp = boolean_choice(itmp, L_EXEC, -1, choices);
  
			   FREE(choices[0]);
			   FREE(choices[1]);
#ifndef NEVER_ALLOW_REMOTE_EXEC
			   FREE(choices[2]);
#endif /* NEVER_ALLOW_REMOTE_EXEC */
			   switch(itmp) {
			      case 0:
				  local_exec = FALSE;
				  local_exec_on_local_files = FALSE;
				  response = ' ';
				  break;
			      case 1:
				  local_exec = FALSE;
				  local_exec_on_local_files = TRUE;
				  response = ' ';
				  break;
#ifndef NEVER_ALLOW_REMOTE_EXEC
			      case 2:
				  local_exec = TRUE;
				  local_exec_on_local_files = FALSE;
				  response = ' ';
				  break;
#endif /* NEVER_ALLOW_REMOTE_EXEC */
			  } /* end switch */
			} else {
			   option_statusline(CHANGE_OF_SETTING_DISALLOWED);
			}
			response = ' ';
			break;
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */

		case '>':
                        if (!no_option_save) {
                            option_statusline(SAVING_OPTIONS);
                            if(save_rc())
				option_statusline(OPTIONS_SAVED);
			    else 
				HTAlert(OPTIONS_NOT_SAVED);

                        } else {
			    option_statusline(R_TO_RETURN_TO_LYNX);
			    /* change response so that we don't exit
			     * the options menu 
			     */
			    response = ' ';
			} 
			break;

		case 'r':
		case 'R':
			break;

		default:
			if (!no_option_save) {
			    option_statusline(SAVE_OR_R_TO_RETURN_TO_LYNX);
			} else {
			    option_statusline(R_TO_RETURN_TO_LYNX);
			}
	    }  /* end switch */
    }  /* end while */

    term_options = FALSE;
    signal(SIGINT, cleanup_sig);
}

/* take a boolean status and prompt the user for a new status
 * and return it
 */

PRIVATE int boolean_choice ARGS4(
	int,		status,
	int,		line,
	int,		column,
	char **,	choices)
{
    int response = 0;
    int number = 0;
    int col = (column >= 0 ? column : COL_OPTION_VALUES);
	
    for (; choices[number] != NULL; number++)
	;  /* empty loop body */

    number--;

    option_statusline(ACCEPT_DATA);
    /*
     *  Highlight the current selection.
     */
    move(line, col);
    standout();
    addstr(choices[status]);

    standend();
    option_statusline(ANY_KEY_CHANGE_RET_ACCEPT);
    standout();

    while (1) {
	move(line, col);
	response = LYgetch();
	if (term_options || response == 7 || response == 3)
	    response = '\n';
	if (response != '\n' && response != '\r') {
	    if (status == number)
		status = 0;  /* go over the top and around */
	    else
		status++;
	    addstr(choices[status]);
	    refresh();
	} else {
	    /*
	     *  Unhighlight selection.
	     */
	    move(line, col);
	    standend();
	    addstr(choices[status]);

	    option_statusline(VALUE_ACCEPTED);
	     return(status);
	}
    }
}

PRIVATE void terminate_options ARGS1(int,sig)
{
    term_options=TRUE;
    /* Reassert the AST */
    signal(SIGINT, terminate_options);
#ifdef VMS
    /* refresh the screen to get rid of the "interrupt" message */
    if (!dump_output_immediately) {
	clearok(curscr, TRUE);
	refresh();
    }
#endif /* VMS */
}

/*
 *  Multi-Bookmark On-Line editing support. - FMG & FM
 */
PUBLIC void edit_bookmarks NOARGS
{
    int response = 0, def_response = 0, ch;
    int MBM_current = 1;
#define	MULTI_OFFSET 8
    int a; /* misc counter */
    char MBM_tmp_line[256]; /* buffer for LYgetstr */
    
    /*
     *  We need (MBM_V_MAXFILES + MULTI_OFFSET) lines to display
     *  the whole list at once.  Otherwise break it up into two
     *  segments.  We know it won't be less than that because
     *  'o'ptions needs 23-24 at LEAST.
     */
    term_options = FALSE;
    signal(SIGINT, terminate_options);

draw_bookmark_list:
    clear(); 
    move(0, 5);
    if (bold_H1 || bold_headers)
        start_bold();
    if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET))
	printw("Editing Bookmark DESCRIPTION and FILEPATH (%d of 2)",
		MBM_current);
    else
        printw("         Editing Bookmark DESCRIPTION and FILEPATH");
    if (bold_H1 || bold_headers)
        stop_bold();

    if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET)) {
	for (a = ((MBM_V_MAXFILES/2 + 1) * (MBM_current - 1));
                      a <= ((float)MBM_V_MAXFILES/2 * MBM_current); a++) {
	    move((3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current - 1)), 5);
	    printw("%c : %s", (a + 'A'),
		   (!MBM_A_subdescript[a] ? "" : MBM_A_subdescript[a]));
	    move((3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current - 1)), 35);
	    printw("| %s",
		   (!MBM_A_subbookmark[a] ? "" : MBM_A_subbookmark[a]));
        }
    } else {
	for (a = 0; a <= MBM_V_MAXFILES; a++) {
	    move(3 + a, 5);
	    printw("%c : %s", (a + 'A'),
		   (!MBM_A_subdescript[a] ? "" : MBM_A_subdescript[a]));
	    move(3 + a, 35);
	    printw("| %s",
		   (!MBM_A_subbookmark[a] ? "" : MBM_A_subbookmark[a]));
	}
    }

    /*
     *  Only needed when we have 2 screens.
     */
    if (LYlines < MBM_V_MAXFILES + MULTI_OFFSET) {
       move((LYlines - 4), 0);
       start_reverse();
       addstr(MULTIBOOKMARKS_MOVE);
       stop_reverse();
    }

    move((LYlines - 3), 0);
    if (!no_option_save) {
        addstr("'");
	standout();
	addstr(">");
	standend();
	addstr("'");
	addstr(TO_SAVE_SEGMENT);
    }
    addstr(OR_SEGMENT);
    addstr("'");
    standout();
    addstr("^G");
    standend();
    addstr("'");
    addstr(TO_RETURN_SEGMENT);

    while (!term_options &&
           !LYisNonAlnumKeyname(response, LYK_PREV_DOC) &&
	   response != 7 && response != 3 &&
	   response != '>') {

	move((LYlines - 2), 0);
	start_reverse();
	addstr("Letter: ");
	stop_reverse();

	refresh();
        response = (def_response ? def_response : LYgetch());
	def_response = 0;

	/*
	 *  Check for a cancel.
	 */
	if (term_options ||
	    response == 7 || response == 3 ||
	    LYisNonAlnumKeyname(response, LYK_PREV_DOC))
	    continue;

	/*
	 *  Check for a save.
	 */
	if (response == '>') {
	    if (!no_option_save) {
		option_statusline(SAVING_OPTIONS);
		if (save_rc())
		    option_statusline(OPTIONS_SAVED);
		else 
		    HTAlert(OPTIONS_NOT_SAVED);
	    } else {
		option_statusline(R_TO_RETURN_TO_LYNX);
		/*
		 *  Change response so that we don't exit
		 *  the options menu.
		 */
		response = ' ';
	    }
	    continue;
	}

	/*
	 *  Check for a refresh.
	 */
	if (LYisNonAlnumKeyname(response, LYK_REFRESH)) {
	    clearok(curscr, TRUE);
	    continue;
	}

	/*
	 *  Move between the screens - if we can't show it all at once.
	 */
	if ((response == ']' ||
	     LYisNonAlnumKeyname(response, LYK_NEXT_PAGE)) &&
	    LYlines < (MBM_V_MAXFILES + MULTI_OFFSET)) {
	    MBM_current++;
	    if (MBM_current >= 3)
		MBM_current = 1;
	    goto draw_bookmark_list;
	}
	if ((response == '[' ||
	     LYisNonAlnumKeyname(response, LYK_PREV_PAGE)) &&
	    LYlines < (MBM_V_MAXFILES + MULTI_OFFSET)) {
	    MBM_current--;
	    if (MBM_current <= 0)
		MBM_current = 2;
	    goto draw_bookmark_list;
	}

	/*
	 *  Instead of using 26 case statements, we set up
         *  a scan through the letters and edit the lines
         *  that way.
         */
	for (a = 0; a <= MBM_V_MAXFILES; a++) {
	    if ((TOUPPER(response) - 'A') == a) {
		if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET)) {
		    if (MBM_current == 1 && a > (MBM_V_MAXFILES/2)) {
		        MBM_current = 2;
		        def_response = response;
			goto draw_bookmark_list;
		    }
		    if (MBM_current == 2 && a < (MBM_V_MAXFILES/2)) {
		        MBM_current = 1;
		        def_response = response;
			goto draw_bookmark_list;
		    }
		}
		option_statusline(ACCEPT_DATA);

		if (a > 0) {
		    standout();
		    if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET))
    		        move(
			 (3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current - 1)),
			     9);
		    else
    		        move((3 + a), 9);
		    strcpy(MBM_tmp_line,
    		           (!MBM_A_subdescript[a] ?
			   		       "" : MBM_A_subdescript[a]));
		    ch = LYgetstr(MBM_tmp_line, VISIBLE,
	    		          sizeof(MBM_tmp_line), NORECALL);
		    standend();

		    if (strlen(MBM_tmp_line) < 1) {
		        FREE(MBM_A_subdescript[a]);
		    } else {
		        StrAllocCopy(MBM_A_subdescript[a], MBM_tmp_line);
		    }
		    if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET))
    			move(
			 (3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current - 1)),
			     5);
		    else
    			move((3 + a), 5);
    		    printw("%c : %s", (a + 'A'),
    			   (!MBM_A_subdescript[a] ?
			       		"" : MBM_A_subdescript[a]));
		    clrtoeol();
	   	    refresh();
		}

		if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET))
    		    move((3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current - 1)),
		    	 35);
		else
    		    move((3 + a), 35);
    		printw("| ");

		standout();
		strcpy(MBM_tmp_line,
    		       (!MBM_A_subbookmark[a] ? "" : MBM_A_subbookmark[a]));
		ch = LYgetstr(MBM_tmp_line, VISIBLE,
	    		      sizeof(MBM_tmp_line), NORECALL);
		standend();

		if (*MBM_tmp_line == '\0') {
		    if (a == 0)
		        StrAllocCopy(MBM_A_subbookmark[a], bookmark_page);
		    else
		        FREE(MBM_A_subbookmark[a]);
		} else {
		    StrAllocCopy(MBM_A_subbookmark[a], MBM_tmp_line);
		    if (a == 0) {
		        StrAllocCopy(bookmark_page, MBM_A_subbookmark[a]);
		    }
		}
		if (LYlines < (MBM_V_MAXFILES + MULTI_OFFSET))
    		    move((3 + a) - ((MBM_V_MAXFILES/2 + 1)*(MBM_current-1)),
		    	 35);
		else
    		    move((3 + a), 35);
    		printw("| %s", (!MBM_A_subbookmark[a] ?
						   "" : MBM_A_subbookmark[a]));
	   	clrtoeol();
		move(LYlines-1, 0);
		clrtoeol();
		break;
	    }
	}  /* end for */
    } /* end while */

    term_options = FALSE;
    signal(SIGINT, cleanup_sig);
}

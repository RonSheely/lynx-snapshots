#include "HTUtils.h"
#include "tcp.h"
#include "HTFTP.h"
#include "LYUtils.h"
#include "LYrcFile.h"
#include "LYStrings.h"
#include "LYGlobalDefs.h"
#include "LYCharSets.h"
#include "LYBookmark.h"

#include "LYLeaks.h"

PUBLIC void read_rc NOPARAMS
{
    char line_buffer[256];
    char rcfile[256];
    FILE *fp;
    char *cp, *cp2;
    int number_sign;
    char MBM_line[256];
    int  MBM_counter;
    char *MBM_cp2, *MBM_cp1;
    int  MBM_i2;

    /*
     *  Make an RC file name.
     */
#ifdef DJGPP
	 sprintf(rcfile, "%s/lynx.rc", Home_Dir());
#else
#ifdef VMS
    sprintf(rcfile, "sys$login:.lynxrc");
#else
    sprintf(rcfile, "%s/.lynxrc", Home_Dir());
#endif /* VMS */
#endif /* DJGPP */

    /*
     *  Open the RC file for reading.
     */
    if ((fp = fopen(rcfile, "r")) == NULL) {
	return;
    }

    /*
     *  Process the entries.
     */
    while (fgets(line_buffer, 256, fp) != NULL) {
	/*
	 *  Remove the /n from the end of the line.
	 */
	if (line_buffer[0] && line_buffer[strlen(line_buffer)-1] == '\n')
	    line_buffer[strlen(line_buffer)-1] = '\0';

	/*
	 *  Remove any trailing white space.
	 */
	while (line_buffer[0] && isspace(line_buffer[strlen(line_buffer)-1]))
	    line_buffer[strlen(line_buffer)-1] = '\0';
	
	/*
	 *  Skip any comment or blank lines.
	 */
	if (line_buffer[0] == '\0' || line_buffer[0] == '#')
	    continue;

        /*
	 *  Find the line position of the number sign if there is one.
	 */
        if ((cp = (char *)strchr(line_buffer, '#')) == NULL)
	    number_sign = 999;
	else
	    number_sign = cp - line_buffer;

        /*
	 *  File editor.
	 */
	if (!system_editor &&
	    (cp = LYstrstr(line_buffer, "file_editor")) != NULL &&
	    cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
	 	cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
 	    StrAllocCopy(editor, cp);

	/*
	 *  Default bookmark file.
	 */
	} else if ((cp = LYstrstr(line_buffer, "bookmark_file")) != NULL &&
		    cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp,'=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */

            /*
	     *  Since this is the "Default Bookmark File", we save it
	     *  as a globals, and as the first MBM_A_subbookmark entry.
	     */
	    StrAllocCopy(bookmark_page, cp);
	    StrAllocCopy(BookmarkPage, cp);
            StrAllocCopy(MBM_A_subbookmark[0], cp);
            StrAllocCopy(MBM_A_subdescript[0], MULTIBOOKMARKS_DEFAULT);

	/*
	 *  Multiple (sub)bookmark support settings.
	 */
        } else if ((cp = LYstrstr(line_buffer, "sub_bookmarks")) != NULL &&
                   cp-line_buffer < number_sign) {

           if ((cp2 = (char *)strchr(cp, '=')) != NULL)
                cp = (cp2 + 1);
           while (isspace(*cp))
	       cp++;  /* get rid of spaces */
           if (!strncasecomp(cp, "standard", 8)) {
              LYMultiBookmarks = TRUE;
              LYMBMAdvanced = FALSE;
           } else if (!strncasecomp(cp, "advanced", 8)) {
              LYMultiBookmarks = TRUE;
              LYMBMAdvanced = TRUE;
           } else {
              LYMultiBookmarks = FALSE;
	   }

	/*
	 *  Multiple (sub)bookmark definitions and descriptions.
	 */
	} else if ((cp = LYstrstr(line_buffer, "multi_bookmark")) != NULL &&
                   cp-line_buffer < number_sign) {

            /*
	     *  Found the root, now cycle through all the
             *  possible spaces and match specific ones.
             */
            for (MBM_counter = 1;
	         MBM_counter <= MBM_V_MAXFILES; MBM_counter++) {
                sprintf(MBM_line, "multi_bookmark%c", (MBM_counter + 'A'));

                if ((cp = LYstrstr(line_buffer, MBM_line)) != NULL &&
                    cp-line_buffer < number_sign) {
                    if ((MBM_cp1 = (char *)strchr(cp, '=')) == NULL) {
                        break;
                    } else {
                        if ((MBM_cp2 = (char *)strchr(cp, ',')) == NULL) {
                            break;
                        } else {
                            MBM_i2 = 0;
			    /*
			     *  skip over the '='.
			     */
                            MBM_cp1++;
                            while (MBM_cp1 && MBM_cp1 != MBM_cp2) {
                                /*
				 *  Skip spaces.
				 */
                                if (isspace(*MBM_cp1)) {
                                    MBM_cp1++;
                                    continue;
                                } else {
                                    MBM_line[MBM_i2++] = *MBM_cp1++;
				}
			    }
                            MBM_line[MBM_i2++] = '\0';

                            StrAllocCopy(MBM_A_subbookmark[MBM_counter],
			    		 MBM_line);

                            /*
			     *  Now get the description ',' and ->.
			     */
                            MBM_cp1 = (char *)strchr(cp, ',');

                            MBM_i2 = 0;
			    /*
			     *  Skip over the ','.
			     */
                            MBM_cp1++;
                            /*
			     *  Eat spaces in front of description.
			     */
                            while (isspace(*MBM_cp1))
                                MBM_cp1++;
                            while (*MBM_cp1)
                                MBM_line[MBM_i2++] = *MBM_cp1++;
                            MBM_line[MBM_i2++] = '\0';

                            StrAllocCopy(MBM_A_subdescript[MBM_counter],
			    		 MBM_line);

                            break;
			}
                    }
		}
	    }

	/*
	 * FTP/file sorting method.
	 */
	} else if ((cp = LYstrstr(line_buffer,
				  "file_sorting_method")) != NULL &&
		   cp-line_buffer < number_sign) {

	   if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	   while (isspace(*cp))
	       cp++;  /* get rid of spaces */
	   if (!strncasecomp(cp, "BY_FILENAME", 11))
		HTfileSortMethod = FILE_BY_NAME;
	   else if (!strncasecomp(cp, "BY_TYPE", 7))
		HTfileSortMethod = FILE_BY_TYPE;
	   else if (!strncasecomp(cp, "BY_SIZE", 7))
		HTfileSortMethod = FILE_BY_SIZE;
	   else if (!strncasecomp(cp, "BY_DATE", 7))
		HTfileSortMethod = FILE_BY_DATE;

	/*
	 *  Personal mail address.
	 */
        } else if ((cp = LYstrstr(line_buffer,
				  "personal_mail_address")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    StrAllocCopy(personal_mail_address, cp);

	/*
	 *  Searching type.
	 */
	} else if ((cp = LYstrstr(line_buffer,
				  "case_sensitive_searching")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (!strncasecomp(cp, "on", 2))
	        case_sensitive = TRUE;
	    else
	        case_sensitive = FALSE;

	/*
	 *  Character set.
	 */
	} else if ((cp = LYstrstr(line_buffer, "character_set")) != NULL &&
		   cp-line_buffer < number_sign) {

	    int i = 0;

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
            for (; LYchar_set_names[i]; i++) {
                if (!strncmp(cp, LYchar_set_names[i], strlen(cp))) {
                    current_char_set=i;
		    HTMLSetRawModeDefault(i);
                    break;
                }
	    }

	/*
	 *  Preferred language.
	 */
	} else if ((cp = LYstrstr(line_buffer,
				  "preferred_language")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    StrAllocCopy(language, cp);

	/*
	 *  Preferred charset.
	 */
	} else if ((cp = LYstrstr(line_buffer,
				  "preferred_charset")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    StrAllocCopy(pref_charset, cp);

	/*
	 * VI keys.
	 */
	} else if ((cp = LYstrstr(line_buffer, "vi_keys")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char * )strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (!strncasecomp(cp, "on", 2))
	        vi_keys = TRUE;
	    else
	        vi_keys = FALSE;

	/*
	 *  EMACS keys.
	 */
        } else if ((cp = LYstrstr(line_buffer, "emacs_keys")) != NULL &&
                   cp-line_buffer < number_sign) {

            if ((cp2 = (char *)strchr(cp, '=')) != NULL)
                cp = cp2 + 1;
            while (isspace(*cp))
	        cp++;  /* get rid of spaces */
            if (!strncasecomp(cp, "on", 2))
                emacs_keys = TRUE;
            else
                emacs_keys=FALSE;

	/*
	 *  Show dot files.
	 */
	} else if ((cp = LYstrstr(line_buffer, "show_dotfiles")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char * )strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (!strncasecomp(cp, "on", 2))
	        show_dotfiles = TRUE;
	    else
	        show_dotfiles = FALSE;

	/*
	 *  Select popups.
	 */
	} else if ((cp = LYstrstr(line_buffer, "select_popups")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char * )strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (!strncasecomp(cp, "off", 3))
	        LYSelectPopups = FALSE;
	    else
	        LYSelectPopups = TRUE;

	/*
	 *  Keypad mode.
	 */
	} else if ((cp = LYstrstr(line_buffer, "keypad_mode")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (LYstrstr(cp,"LINKS_ARE_NUMBERED"))
	        keypad_mode = LINKS_ARE_NUMBERED;
	    else
	        keypad_mode = NUMBERS_AS_ARROWS;

	/*
	 *  Linedit mode.
	 */
	} else if ((cp = LYstrstr(line_buffer, "lineedit_mode")) != NULL &&
		   cp-line_buffer < number_sign) {

	    int i = 0;

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
            for (; LYLineeditNames[i]; i++) {
                if (!strncmp(cp, LYLineeditNames[i], strlen(cp))) {
                    current_lineedit = i;
                    break;
                }
	    }

#ifdef DIRED_SUPPORT
	/*
	 *  List directory style.
	 */
	} else if ((cp = LYstrstr(line_buffer, "dir_list_style")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp,'=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (LYstrstr(cp, "FILES_FIRST") != NULL) {
	        dir_list_style = FILES_FIRST;
	    } else if (LYstrstr(cp,"DIRECTORIES_FIRST") != NULL) {
	        dir_list_style = 0;
	    } else {
	        dir_list_style = MIXED_STYLE;
	    }
#endif /* DIRED_SUPPORT */

	/*
	 *  User mode.
	 */
	} else if ((cp = LYstrstr(line_buffer, "user_mode")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (LYstrstr(cp, "ADVANCED") != NULL) {
	        user_mode = ADVANCED_MODE;
	    } else if (LYstrstr(cp,"INTERMEDIATE") != NULL) {
	        user_mode = INTERMEDIATE_MODE;
	    } else {
	        user_mode = NOVICE_MODE;
	    }

#ifdef ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS
        /*
	 *  Local execution mode - all links.
	 */
	} else if ((cp = LYstrstr(line_buffer,
				  "run_all_execution_links")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */

	    if (!strncasecomp(cp, "on", 2))
	        local_exec = TRUE;
	     else
	        local_exec = FALSE;

        /*
	 *  Local execution mode - only links in local files.
	 */
	} else if ((cp = LYstrstr(line_buffer,
			"run_execution_links_on_local_files")) != NULL &&
		   cp-line_buffer < number_sign) {

	    if ((cp2 = (char *)strchr(cp, '=')) != NULL)
		cp = cp2 + 1;
	    while (isspace(*cp))
	        cp++;  /* get rid of spaces */
	    if (!strncasecomp(cp, "on", 2))
	        local_exec_on_local_files = TRUE;
	    else
	        local_exec_on_local_files=FALSE;
#endif /* ALLOW_USERS_TO_CHANGE_EXEC_WITHIN_OPTIONS */

	} /* end of if */

    } /* end of while */
 
    fclose(fp);
} /* big end */
	
PUBLIC int save_rc NOPARAMS
{
    char rcfile[256];
    FILE *fp;
    int i;
    int MBM_c;

    /*
     *  Make a name.
     */
#ifdef DJGPP
	 sprintf(rcfile, "%s/lynx.rc", Home_Dir());
#else
#ifdef VMS
    sprintf(rcfile, "sys$login:.lynxrc");
#else
    sprintf(rcfile, "%s/.lynxrc", Home_Dir());
#endif /* VMS */
#endif /* DJGPP */
    
    /*
     *  Open the file for write.
     */
    if ((fp = fopen(rcfile, "w")) == NULL) {
	return FALSE;
    }

    /*
     *  Header.
     */
    fprintf(fp, "# Lynx User Defaults File\n\n");

    /*
     *  File editor
     */
    fprintf(fp, "\
# file_editor specifies the editor to be invoked when editing local files\n\
# or sending mail.  If no editor is specified, then file editing is disabled\n\
# unless it is activated from the command line, and the built-in line editor\n\
# will be used for sending mail.\n");
    fprintf(fp, "file_editor=%s\n\n", (editor ? editor : ""));

    /*
     *  Default bookmark file.
     */
    fprintf(fp, "\
# bookmark_file specifies the name and location of the default bookmark\n\
# file into which the user can paste links for easy access at a later\n\
# date.\n");
    fprintf(fp, "bookmark_file=%s\n\n", (bookmark_page ? bookmark_page : ""));

    /*
     *  Multiple (sub)bookmark support settings.
     */
    fprintf(fp, "\
# If sub_bookmarks is not turned \"off\", and multiple bookmarks have\n\
# been defined (see below), then all bookmark operations will first\n\
# prompt the user to select an active sub-bookmark file.  If the default\n\
# Lynx bookmark_file is defined (see above), it will be used as the\n\
# default selection.  When this option is set to \"advanced\", and the\n\
# user mode is advanced, the 'v'iew bookmark command will invoke a\n\
# statusline prompt instead of the menu seen in novice and intermediate\n\
# user modes.  When this option is set to \"standard\", the menu will be\n\
# presented regardless of user mode.\n");
    fprintf(fp, "sub_bookmarks=%s\n\n", (LYMultiBookmarks ?
          				   (LYMBMAdvanced ?
					       "advanced" : "standard")
					       		  : "off"));

    /*
     *  Multiple (sub)bookmark definitions and descriptions.
     */
    fprintf(fp, "\
# The following allow you to define sub-bookmark files and descriptions.\n\
# The format is multi_bookmark<capital_letter>=<filename>,<description>\n\
# Up to 26 bookmark files (for the English capital letters) are allowed.\n\
# We start with \"multi_bookmarkB\" since 'A' is the default (see above).\n");
    for (MBM_c = 1; MBM_c <= MBM_V_MAXFILES; MBM_c++)
       fprintf(fp, "multi_bookmark%c=%s%s%s\n",
       		   (MBM_c + 'A'),
		   (MBM_A_subbookmark[MBM_c] ?
		    MBM_A_subbookmark[MBM_c] : ""),
		   (MBM_A_subbookmark[MBM_c] ?
		   			 "," : ""),
		   (MBM_A_subdescript[MBM_c] ?
		    MBM_A_subdescript[MBM_c] : ""));
    fprintf(fp, "\n");

    /*
     *  FTP/file sorting method.
     */
    fprintf(fp, "\
# The file_sorting_method specifies which value to sort on when viewing\n\
# file lists such as FTP directories.  The options are:\n\
#    BY_FILENAME -- sorts on the name of the file\n\
#    BY_TYPE     -- sorts on the type of the file\n\
#    BY_SIZE     -- sorts on the size of the file\n\
#    BY_DATE     -- sorts on the date of the file\n");
    fprintf(fp, "file_sorting_method=%s\n\n",
     		(HTfileSortMethod == FILE_BY_NAME ? "BY_FILENAME"
				    		  : 
	 	(HTfileSortMethod == FILE_BY_SIZE ? "BY_SIZE"
						  :
		(HTfileSortMethod == FILE_BY_TYPE ? "BY_TYPE"
						  : "BY_DATE"))));

    /*
     *  Personal mail address.
     */
    fprintf(fp, "\
# personal_mail_address specifies your personal mail address.  The\n\
# address will be sent during HTTP file transfers for authorization and\n\
# logging purposes, and for mailed comments.\n\
# If you do not want this information given out, set the NO_FROM_HEADER\n\
# to TRUE in lynx.cfg, or use the -nofrom command line switch.  You also\n\
# could leave this field blank, but then you won't have if it included in\n\
# your mailed comments.\n");
    fprintf(fp, "personal_mail_address=%s\n\n",
    		(personal_mail_address ? personal_mail_address : ""));

    /*
     *  Searching type.
     */
    fprintf(fp, "\
# If case_sensitive_searching is \"on\" then when the user invokes a search\n\
# using the 's' or '/' keys, the search performed will be case sensitive\n\
# instead of case INsensitive.  The default is usually \"off\".\n");
    fprintf(fp, "case_sensitive_searching=%s\n\n",
  		(case_sensitive ? "on" : "off"));

    /*
     *  Character set.
     */
    fprintf(fp, "\
# The character_set definition controls the representation of 8 bit\n\
# characters for your terminal.  If 8 bit characters do not show up\n\
# correctly on your screen you may try changing to a different 8 bit\n\
# set or using the 7 bit character approximations.\n\
# Current valid characters sets are:\n");
    for (i = 0; LYchar_set_names[i]; i++)
	fprintf(fp, "#    %s\n", LYchar_set_names[i]);
    fprintf(fp, "character_set=%s\n\n", LYchar_set_names[current_char_set]);


    /*
     *  Preferred language.
     */
    fprintf(fp, "\
# preferred_language specifies the language in MIME notation (e.g., en,\n\
# fr) which Lynx will indicate you prefer in requests to http servers.\n\
# If a file in that language is available, the server will send it.\n\
# Otherwise, the server will send the file in it's default language.\n");
    fprintf(fp, "preferred_language=%s\n\n", (language ? language : ""));

    /*
     *  Preferred charset.
     */
    fprintf(fp, "\
# preferred_charset specifies the character set in MIME notation (e.g.,\n\
# ISO-8859-2, ISO-8859-5) which Lynx will indicate you prefer in requests\n\
# to http servers using an Accept-Charset header.  The value should NOT\n\
# include ISO-8859-1 or US-ASCII, since those values are always assumed by\n\
# default.  If a file in that character set is available, the server will\n\
# send it.  Otherwise, the server will send the file in ISO-8859-1 or\n\
# US-ASCII.\n");
    fprintf(fp, "preferred_charset=%s\n\n",
    		(pref_charset ? pref_charset : ""));

    /*
     *  VI keys.
     */
    fprintf(fp, "\
# If vi_keys is set to \"on\", then the normal VI movement keys:\n\
#   j = down    k = up\n\
#   h = left    l = right\n\
# will be enabled.  These keys are only lower case.\n\
# Capital 'H', 'J' and 'K will still activate help, jump shortcuts,\n\
# and the keymap display, respectively.\n");
     fprintf(fp, "vi_keys=%s\n\n", (vi_keys ? "on" : "off"));

    /*
     *  EMACS keys.
     */
    fprintf(fp, "\
# If emacs_keys is to \"on\" then the normal EMACS movement keys:\n\
#   ^N = down    ^P = up\n\
#   ^B = left    ^F = right\n\
# will be enabled.\n");
    fprintf(fp, "emacs_keys=%s\n\n", (emacs_keys ? "on" : "off"));

    /*
     *  Show dot files.
     */
    fprintf(fp, "\
# show_dotfiles specifies that the directory listing should include\n\
# \"hidden\" (dot) files/directories.  If set \"on\", this will be\n\
# honored only if enabled via userdefs.h and/or lynx.cfg, and not\n\
# restricted via a command line switch.  If display of hidden files\n\
# is disabled, creation of such files via Lynx also is disabled.\n");
    fprintf(fp, "show_dotfiles=%s\n\n", (show_dotfiles ? "on" : "off"));

    /*
     *  Select popups.
     */
    fprintf(fp, "\
# select_popups specifies whether the OPTIONs in a SELECT block which\n\
# lacks a MULTIPLE attribute are presented as a vertical list of radio\n\
# buttons or via a popup menu.  Note that if the MULTIPLE attribute is\n\
# present in the SELECT start tag, Lynx always will create a vertical list\n\
# of checkboxes for the OPTIONs.  A value of \"on\" will set popup menus\n\
# as the default while a value of \"off\" will set use of radio boxes.\n\
# The default can be overridden via the -popup command line toggle.\n");
    fprintf(fp, "select_popups=%s\n\n", (LYSelectPopups ? "on" : "off"));

    /*
     *  Keypad mode.
     */
    fprintf(fp, "\
# If keypad_mode is set to \"NUMBERS_AS_ARROWS\", then the numbers on\n\
# your keypad when the numlock is on will act as arrow keys:\n\
#             8 = Up Arrow\n\
#   4 = Left Arrow    6 = Right Arrow\n\
#             2 = Down Arrow\n\
# If keypad_mode is set to \"LINKS_ARE_NUMBERED\", then numbers will\n\
# appear next to each link and numbers are used to select links.\n\
# NOTE: Some fixed format documents may look disfigured when\n\
# \"LINKS_ARE_NUMBERED\" is enabled.\n");
    fprintf(fp, "keypad_mode=%s\n\n",
    		(keypad_mode==NUMBERS_AS_ARROWS ? 
			    "NUMBERS_AS_ARROWS" : "LINKS_ARE_NUMBERED"));
 
    /*
     *  Lineedit mode.
     */
    fprintf(fp, "\
# linedit_mode specifies the key binding used for inputting strings in\n\
# prompts and forms.  If lineedit_mode is set to \"Default Binding\" then\n\
# the following control characters are used for moving and deleting:\n\
#\n\
#              Prev  Next       Enter = Accept input\n\
#    Move char: <-    ->        ^G    = Cancel input\n\
#    Move word: ^P    ^N        ^U    = Erase line\n\
#  Delete char: ^H    ^R        ^A    = Beginning of line\n\
#  Delete word: ^B    ^F        ^E    = End of line\n\
#\n\
# Current lineedit modes are:\n");
    {
	char **bindings = LYLineeditNames;
	while (*bindings) {
	    fprintf(fp, "#    %s\n", *bindings);
	    bindings++;
	}
    }
    fprintf(fp, "lineedit_mode=%s\n\n", LYLineeditNames[current_lineedit]);

#ifdef DIRED_SUPPORT
    /*
     *  List directory style.
     */
    fprintf(fp, "\
# dir_list_styles specifies the directory list style under DIRED_SUPPORT\n\
# (if implemented).  The default is \"MIXED_STYLE\", which sorts both\n\
# files and directories together.  \"FILES_FIRST\" lists files first and\n\
# \"DIRECTORIES_FIRST\" lists directories first.\n");
    fprintf(fp, "dir_list_style=%s\n\n",
    		(dir_list_style==FILES_FIRST ? "FILES_FIRST"
					     : 
		(dir_list_style==MIXED_STYLE ? "MIXED_STYLE"
					     : "DIRECTORIES_FIRST")));
#endif /* DIRED_SUPPORT */

    /*
     *  User mode.
     */
    fprintf(fp, "\
# user_mode specifies the users level of knowledge with Lynx.  The\n\
# default is \"NOVICE\" which displays two extra lines of help at the\n\
# bottom of the screen to aid the user in learning the basic Lynx\n\
# commands.  Set user_mode to \"INTERMEDIATE\" to turn off the extra info.\n\
# Use \"ADVANCED\" to see the URL of the currently selected link at the\n\
# bottom of the screen.\n");
    fprintf(fp, "user_mode=%s\n\n",
    		(user_mode == NOVICE_MODE ? "NOVICE" : 
			 (user_mode == ADVANCED_MODE ? 
			 		  "ADVANCED" : "INTERMEDIATE")));

#if defined(EXEC_LINKS) || defined(EXEC_SCRIPTS) 
    /*
     *  Local execution mode - all links.
     */
    fprintf(fp, "\
# If run_all_execution_links is set \"on\" then all local exection links\n\
# will be executed when they are selected.\n\
#\n\
# WARNING - This is potentially VERY dangerous.  Since you may view\n\
#           information that is written by unknown and untrusted sources\n\
#           there exists the possibility that Trojan horse links could be\n\
#           written.  Trojan horse links could be written to erase files\n\
#           or compromise security.  This should only be set to \"on\" if\n\
#           you are viewing trusted source information.\n");
    fprintf(fp, "run_all_execution_links=%s\n\n",
    		(local_exec ? "on" : "off"));

    /*
     *  Local execution mode - only links in local files.
     */
    fprintf(fp, "\
# If run_execution_links_on_local_files is set \"on\" then all local\n\
# execution links that are found in LOCAL files will be executed when they\n\
# are selected.  This is different from run_all_execution_links in that\n\
# only files that reside on the local system will have execution link\n\
# permissions.\n\
#\n\
# WARNING - This is potentially dangerous.  Since you may view\n\
#           information that is written by unknown and untrusted sources\n\
#           there exists the possibility that Trojan horse links could be\n\
#           written.  Trojan horse links could be written to erase files\n\
#           or compromise security.  This should only be set to \"on\" if\n\
#           you are viewing trusted source information.\n");
    fprintf(fp, "run_execution_links_on_local_files=%s\n\n",
		(local_exec_on_local_files ? "on" : "off"));
#endif /* defined(EXEC_LINKS) || defined(EXEC_SCRIPTS) */

    /*
     *  Close the RC file.
     */
    fclose(fp);

#ifdef VMS
    /*
     *  Get rid of any copies of the .lynxrc file that VMS creates.
     */
    while (remove("sys$login:.lynxrc;-1") == 0) ;
	/*
	 *  Reset version number.
	 */
	rename("sys$login:.lynxrc", "sys$login:.lynxrc;1");
#endif /* VMS */

   return TRUE;
}
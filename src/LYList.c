/*			Lynx Document Reference List Support	      LYList.c
**			====================================
**
**	Author:	FM	Foteos Macrides (macrides@sci.wfbr.edu)
**
*/

#include "HTUtils.h"
#include "tcp.h"
#include "LYUtils.h"
#include "LYList.h"
#include "GridText.h"
#include "LYSignal.h"
#include "LYGlobalDefs.h"
#include "LYCharUtils.h"

#ifdef DIRED_SUPPORT
#include "LYUpload.h"
#include "LYLocal.h"
#endif /* DIRED_SUPPORT */

#include "LYexit.h"
#include "LYLeaks.h"
 
#define FREE(x) if (x) {free(x); x = NULL;}

/* 	showlist - F.Macrides (macrides@sci.wfeb.edu)
**	--------
**  	Create a temporary text/html file with a list of links to
**	HyperText References in the current document.
**
** On entry
**	titles		Set:	if we want titles where available
**			Clear:  we only get addresses.
*/

static char list_filename[256] = "\0";

PUBLIC char * LYlist_temp_url NOARGS
{
    return list_filename;
}

PUBLIC int showlist ARGS2(char **, newfile, BOOLEAN, titles)
{
    int cnt;
    int refs, hidden_links;
    static char tempfile[256];
    static BOOLEAN first = TRUE;
    FILE *fp0;
    char *Address = NULL, *Title = NULL, *cp = NULL;

    refs = HText_sourceAnchors(HTMainText);
    hidden_links = HText_HiddenLinkCount(HTMainText);
    if (refs <= 0 && hidden_links > 0 &&
	LYHiddenLinks != HIDDENLINKS_SEPARATE) {
	_statusline(NO_VISIBLE_REFS_FROM_DOC);
	sleep(MessageSecs);
	return(-1);
    }
    if (refs <= 0 && hidden_links <= 0) {
	_statusline(NO_REFS_FROM_DOC);
	sleep(MessageSecs);
	return(-1);
    }

    if (first) {
	tempname(tempfile,NEW_FILE);
	first = FALSE;
#ifdef VMS
    } else {
	remove(tempfile);  /* Remove duplicates on VMS. */
#endif /* VMS */
    }

    if ((fp0 = fopen(tempfile, "w")) == NULL) {
	_statusline(CANNOT_OPEN_TEMP);
	sleep(MessageSecs);
	return(-1);
    }

    /*
     *  Make the file a URL now.
     */
#if defined (VMS) || defined (DOSPATH)
    sprintf(list_filename, "file://localhost/%s", tempfile);
#else
    sprintf(list_filename, "file://localhost%s", tempfile);
#endif /* VMS */
    StrAllocCopy(*newfile, list_filename);
    LYforce_HTML_mode = TRUE; /* force this file to be HTML */
    LYforce_no_cache = TRUE; /* force this file to be new */


    fprintf(fp0, "<head>\n");
#ifdef EXP_CHARTRANS
    add_META_charset_to_fd(fp0, -1);
#endif
    fprintf(fp0, "<title>%s</title>\n</head>\n<body>\n",
		 LIST_PAGE_TITLE);

    fprintf(fp0, "<h1>You have reached the List Page</h1>\n");
    fprintf(fp0, "<h2>%s Version %s</h2>\n", LYNX_NAME, LYNX_VERSION);

    fprintf(fp0, "  References in this document:<p>\n");
    fprintf(fp0, "<%s compact>\n", (keypad_mode == LINKS_ARE_NUMBERED) ?
    				   "ul" : "ol");
    if (hidden_links > 0) {
        fprintf(fp0, "<lh><em>Visible links:</em>\n");
	if (LYHiddenLinks == HIDDENLINKS_IGNORE)
	    hidden_links = 0;
    }
    for (cnt=1; cnt<=refs; cnt++) {
	HTChildAnchor *child = HText_childNumber(cnt);
	HTAnchor *dest_intl = HTAnchor_followTypedLink((HTAnchor *)child,
						       LINK_INTERNAL);
	HTAnchor *dest = dest_intl ?
	    dest_intl : HTAnchor_followMainLink((HTAnchor *)child);
	HTParentAnchor *parent = HTAnchor_parent(dest);
	char *address =  HTAnchor_address(dest);
	CONST char *title = titles ? HTAnchor_title(parent) : NULL;

	StrAllocCopy(Address, address);
	FREE(address);
	LYEntify(&Address, TRUE);
	if (title && *title) {
	    StrAllocCopy(Title, title);
	    if (*Title)
		LYEntify(&Title, TRUE);
	    else
		FREE(Title);
	    cp = strchr(Address, '#');
	}

        fprintf(fp0, "<li><a href=\"%s\"%s>%s%s%s%s%s</a>\n", Address,
		        dest_intl ? " TYPE=\"internal link\"" : "", 
		        dest_intl ? "(internal) " : "", 
			((HTAnchor*)parent != dest) && Title ? "in " : "",
			(char *)(Title ? Title : Address),
			(Title && cp) ? " - " : "",
                        (Title && cp) ? (cp+1) : "");

	FREE(Address);
	FREE(Title);
    }

    if (hidden_links > 0) {
        if (refs > 0)
	    fprintf(fp0, "\n</%s>\n\n<p>\n",
	    		 (keypad_mode == LINKS_ARE_NUMBERED) ?
							"ul" : "ol");
        fprintf(fp0, "<%s compact>\n", (keypad_mode == LINKS_ARE_NUMBERED) ?
    				       "ul" : "ol continue");
        fprintf(fp0, "<lh><em>Hidden links:</em>\n");
    }

    for (cnt = 0; cnt < hidden_links; cnt++) {
	StrAllocCopy(Address, HText_HiddenLinkAt(HTMainText, cnt));
	LYEntify(&Address, FALSE);
	if (!(Address && *Address)) {
	    FREE(Address);
	    continue;
	}
        fprintf(fp0, "<li><a href=\"%s\">%s</a>\n", Address, Address);

	FREE(Address);
    }

    fprintf(fp0,"\n</%s>\n</body>\n", (keypad_mode == LINKS_ARE_NUMBERED) ?
    				      "ul" : "ol");

    fclose(fp0);
    return(0);
}      


/* 	printlist - F.Macrides (macrides@sci.wfeb.edu)
**	---------
**  	Print a text/plain list of HyperText References
**	in the current document.
**
** On entry
**	titles		Set:	if we want titles where available
**			Clear:  we only get addresses.
*/

PUBLIC void printlist ARGS2(
	FILE *,		fp,
	BOOLEAN,	titles)
{
#ifdef VMS
    extern BOOLEAN HadVMSInterrupt;
#endif /* VMS */
    int cnt;
    int refs, hidden_links;
    char *address = NULL;

    refs = HText_sourceAnchors(HTMainText);
    if (refs <= 0 && LYHiddenLinks != HIDDENLINKS_SEPARATE)
        return;
    hidden_links = HText_HiddenLinkCount(HTMainText);
    if (refs <= 0 && hidden_links <= 0) {
        return;
    } else {
	fprintf(fp, "\n%s\n\n", "References");
	if (hidden_links > 0) {
	    fprintf(fp, "   Visible links:\n");
	    if (LYHiddenLinks == HIDDENLINKS_IGNORE)
		hidden_links = 0;
	}
	for (cnt = 1; cnt <= refs; cnt++) {
	    HTAnchor *dest =
		HTAnchor_followMainLink((HTAnchor *)
					HText_childNumber(cnt));
	    HTParentAnchor * parent = HTAnchor_parent(dest);
	    CONST char * title = titles ? HTAnchor_title(parent) : NULL;

	    address =  HTAnchor_address(dest);
	    fprintf(fp, "%4d. %s%s\n", cnt,
		    ((HTAnchor*)parent != dest) && title ? "in " : "",
		    (char *)(title ? title : address));
	    FREE(address);
#ifdef VMS
	    if (HadVMSInterrupt)
	        break;
#endif /* VMS */
	}

	if (hidden_links > 0)
	    fprintf(fp, "%s   Hidden links:\n", ((refs > 0) ? "\n" : ""));
	for (cnt = 0; cnt < hidden_links; cnt++) {
	    StrAllocCopy(address, HText_HiddenLinkAt(HTMainText, cnt));
	    if (!(address && *address)) {
		FREE(address);
		continue;
	    }
	    fprintf(fp, "%4d. %s\n", ((cnt + 1) + refs), address);
	    FREE(address);
#ifdef VMS
	    if (HadVMSInterrupt)
	        break;
#endif /* VMS */
	}
    }
    return;
}      
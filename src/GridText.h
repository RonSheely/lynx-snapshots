
/*	Specialities of GridText as subclass of HText
*/
#ifndef LYGRIDTEXT_H
#define LYGRIDTEXT_H

#include "HText.h"		/* Superclass */

#ifndef HTFORMS_H
#include "HTForms.h"
#endif /* HTFORMS_H */

#define LY_UNDERLINE_START_CHAR	'\003'
#define LY_UNDERLINE_END_CHAR	'\004'
#define LY_BOLD_START_CHAR	'\005'
#define LY_BOLD_END_CHAR	'\006'
#define LY_SOFT_HYPHEN		'\007'
#define IsSpecialAttrChar(a)  ((a > '\002') && (a < '\010'))

extern int HTCurSelectGroupType;
extern char * HTCurSelectGroupSize;
extern HText * HTMainText;		/* Equivalent of main window */
extern HTParentAnchor * HTMainAnchor;	/* Anchor for HTMainText */

#ifdef SHORT_NAMES
#define HText_childNumber		HTGTChNu
#define HText_canScrollUp		HTGTCaUp
#define HText_canScrollDown		HTGTCaDo
#define HText_scrollUp			HTGTScUp
#define HText_scrollDown		HTGTScDo
#define HText_scrollTop			HTGTScTo
#define HText_scrollBottom		HTGTScBo
#define HText_sourceAnchors		HTGTSoAn
#define HText_setStale			HTGTStal
#define HText_refresh			HTGTRefr
#endif /* SHORT_NAMES */

extern int WWW_TraceFlag;
extern int HTCacheSize;

extern BOOLEAN mustshow;

#if defined(VMS) && defined(VAXC) && !defined(__DECC)
extern int HTVirtualMemorySize;
#endif /* VMS && VAXC && !__DECC */
extern HTChildAnchor * HText_childNumber PARAMS((int n));

/*	Is there any file left?
*/
extern BOOL HText_canScrollUp PARAMS((HText * text));
extern BOOL HText_canScrollDown NOPARAMS;

/*	Move display within window
*/
extern void HText_scrollUp PARAMS((HText * text));	/* One page */
extern void HText_scrollDown PARAMS((HText * text));	/* One page */
extern void HText_scrollTop PARAMS((HText * text));
extern void HText_scrollBottom PARAMS((HText * text));
extern void HText_pageDisplay PARAMS((int line_num, char *target));

extern int HText_LinksInLines PARAMS((HText *text, int line_num, int lines));

extern int HText_sourceAnchors PARAMS((HText * text));
extern void HText_setStale PARAMS((HText * text));
extern void HText_refresh PARAMS((HText * text));
extern char * HText_getTitle NOPARAMS;
extern char * HText_getSugFname NOPARAMS;
extern char * HText_getLastModified NOPARAMS;
extern char * HText_getDate NOPARAMS;
extern char * HText_getServer NOPARAMS;
extern char * HText_getOwner NOPARAMS;
extern void HText_setMainTextOwner PARAMS((CONST char * owner));
extern char * HText_getRevTitle NOPARAMS;
extern void print_wwwfile_to_fd PARAMS((FILE * fp, int is_reply));
extern BOOLEAN HTFindPoundSelector PARAMS((char *selector));
extern int HTGetLinkInfo PARAMS((int number, char **hightext, char **lname));
extern int HTisDocumentSource NOPARAMS;
extern void HTuncache_current_document NOPARAMS;
extern int HText_getTopOfScreen NOPARAMS;
extern int HText_getNumOfLines NOPARAMS;
extern int do_www_search PARAMS((document *doc));
extern char * HTLoadedDocumentURL NOPARAMS;
extern char * HTLoadedDocumentPost_data NOPARAMS;
extern char * HTLoadedDocumentTitle NOPARAMS;
extern BOOLEAN HTLoadedDocumentIsHEAD NOPARAMS;
extern char * HTLoadedDocumentCharset NOPARAMS;
extern void HText_setNodeAnchorBookmark PARAMS((CONST char *bookmark));
extern char * HTLoadedDocumentBookmark NOPARAMS;
extern int HText_LastLineSize PARAMS((HText *me));
extern int HText_PreviousLineSize PARAMS((HText *me));
extern void HText_NegateLineOne PARAMS((HText *text));
extern int HText_getCurrentColumn PARAMS((HText *text));
extern int HText_getMaximumColumn PARAMS((HText *text));
extern void HText_setTabID PARAMS((HText *text, CONST char *name));
extern int HText_getTabIDColumn PARAMS((HText *text, CONST char *name));

/* forms stuff */
extern void HText_beginForm PARAMS((char *action,
				    char *method,
				    char *enctype,
				    char *title));
extern void HText_endForm PARAMS((HText *text));
extern void HText_beginSelect PARAMS((char *name, BOOLEAN multiple, char *len));
extern char * HText_setLastOptionValue PARAMS((HText *text, char *value,
						char *submit_value,
						int order, BOOLEAN checked));
extern int HText_beginInput PARAMS((HText *text, InputFieldData *I));
extern void HText_SubmitForm PARAMS((FormInfo *submit_item, document *doc,
				     char *link_name, char *link_value));
extern void HText_DisableCurrentForm NOPARAMS;
extern void HText_ResetForm PARAMS((FormInfo *form));
extern void HText_activateRadioButton PARAMS((FormInfo *form));

#ifdef CURSES
extern int HText_getTopOfScreen NOPARAMS;
extern int HText_getLines PARAMS((HText * text));
#endif /* CURSES */

extern HTList * search_queries; /* Previous isindex and whereis queries */
extern void HTSearchQueries_free NOPARAMS;
extern void HTAddSearchQuery PARAMS((char *query));

extern void user_message PARAMS((char * message, char * argument));

#define _user_message(msg, arg)	mustshow = TRUE, user_message(msg, arg)

extern void www_user_search PARAMS((int start_line, char *target));

extern void print_crawl_to_fd PARAMS((FILE * fp, char * thelink,
				      char * thetitle));
extern char * stub_HTAnchor_address PARAMS((HTAnchor *me));

extern void HText_setToolbar PARAMS((HText *text));
extern BOOL HText_hasToolbar PARAMS((HText *text));

extern void HText_setNoCache PARAMS((HText *text));
extern BOOL HText_hasNoCacheSet PARAMS((HText *text));

extern void HText_setKcode PARAMS((HText *text, CONST char *charset));

#endif /* LYGRIDTEXT_H */

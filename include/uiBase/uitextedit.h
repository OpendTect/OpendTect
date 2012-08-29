#ifndef uitextedit_h
#define uitextedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.h,v 1.32 2012-08-29 11:14:05 cvsraman Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "undefval.h"

class uiTextEditBody;
class uiTextBrowserBody;
class QTextEdit;
class Timer;

mClass(uiBase) uiTextEditBase : public uiObject
{
public:
    void		readFromFile(const char*,int linecutlen=0);
    bool		saveToFile(const char*,int linelen=0,bool newlns=true);

    const char*		text() const;
    int			nrLines() const;

    int			defaultWidth()		  { return defaultwidth_; }
    void		setDefaultWidth( int w )  { defaultwidth_ = w; }

    int			defaultHeight()		  { return defaultheight_; }
    void		setDefaultHeight( int h ) { defaultheight_ = h; }

    bool		isModified() const;
    void        allowTextSelection(bool);

    void		hideFrame();

protected:
			uiTextEditBase(uiParent*,const char*,uiObjectBody&);

    virtual QTextEdit&	qte()			    = 0;
    const QTextEdit&	qte() const 
			{ return const_cast<uiTextEditBase*>(this)->qte(); }

    int			defaultwidth_;
    int			defaultheight_;
    virtual int		maxLines() const		{ return -1; }

    mutable BufferString result_;
};



mClass(uiBase) uiTextEdit : public uiTextEditBase
{
public:
                        uiTextEdit(uiParent* parnt,const char* nm="Text editor",
				   bool readonly=false);

    void		setText(const char*,bool trigger_notif=false);
    void		append(const char*); 
    Notifier<uiTextEdit> textChanged;

protected:

    virtual QTextEdit&	qte();

private:

    uiTextEditBody*	body_;
    uiTextEditBody&	mkbody(uiParent*,const char*,bool);
};



mClass(uiBase) uiTextBrowser : public uiTextEditBase
{
friend class		i_BrowserMessenger;
public:

                        uiTextBrowser(uiParent*,const char* nm="File browser",
				      int maxlns=mUdf(int),
				      bool forceplaintext=true,
				      bool logmode=false);
			~uiTextBrowser();

    void		setText(const char*);
    void		setHtmlText(const char*);
    void		getHtmlText(BufferString&) const;
    const char*		source() const;
    void		setSource(const char*); 
    void		setMaxLines(int);

    enum		LinkBehavior { None, FollowLocal, FollowAll };
    void		setLinkBehavior( LinkBehavior );

    void		backward();
    void		forward();
    void		home();
    void		reload();
    void		scrollToBottom();
    void		showToolTip(const char*);
    void		recordScrollPos();
    void		restoreScrollPos();

    bool		canGoForward()		{ return cangoforw_; }
    bool		canGoBackward()		{ return cangobackw_; }
    const char* 	lastLink()		{ return lastlink_; }

    Notifier<uiTextBrowser>	goneForwardOrBack;
    Notifier<uiTextBrowser>	linkHighlighted;
    Notifier<uiTextBrowser>	linkClicked;

protected:

    BufferString	textsrc_;
    BufferString	lastlink_;
    bool		cangoforw_;
    bool		cangobackw_;
    bool		forceplaintxt_;
    int			maxlines_;

    virtual int		maxLines() const		{ return maxlines_; }

    virtual QTextEdit&	qte();

    void		readTailCB(CallBacker*);
    Timer*		timer_;
    bool		logviewmode_;
    od_int64		lastlinestartpos_;
    BufferString	lastline_;

private:

    uiTextBrowserBody*	body_;
    uiTextBrowserBody&	mkbody(uiParent*,const char*,bool);
};

#endif


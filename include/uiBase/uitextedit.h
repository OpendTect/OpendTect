#ifndef uitextedit_h
#define uitextedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.h,v 1.23 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "undefval.h"

class uiTextEditBody;
class uiTextBrowserBody;
class QTextEdit;

mClass uiTextEditBase : public uiObject
{
public:
			uiTextEditBase(uiParent*,const char*,uiObjectBody&);

    void		setText(const char*);
    void		readFromFile(const char*,int linecutlen=0);
    bool		saveToFile(const char*,int linelen=0,bool newlns=true);

    const char*		text() const;
    int			nrLines() const;

    int			defaultWidth()		  { return defaultwidth_; }
    void		setDefaultWidth( int w )  { defaultwidth_ = w; }

    int			defaultHeight()		  { return defaultheight_; }
    void		setDefaultHeight( int h ) { defaultheight_ = h; }

    bool		isModified() const;

protected:

    virtual QTextEdit&	qte()			    = 0;
    const QTextEdit&	qte() const 
			{ return const_cast<uiTextEditBase*>(this)->qte(); }

    int			defaultwidth_;
    int			defaultheight_;
    virtual int		maxLines() const		{ return -1; }

    mutable BufferString result_;
};



mClass uiTextEdit : public uiTextEditBase
{
public:
                        uiTextEdit(uiParent* parnt,const char* nm="Text editor",
				   bool readonly=false);

    void		append(const char*); 

protected:

    virtual QTextEdit&	qte();

private:

    uiTextEditBody*	body_;
    uiTextEditBody&	mkbody(uiParent*,const char*,bool);
};



mClass uiTextBrowser : public uiTextEditBase
{
friend class		i_BrowserMessenger;
public:

                        uiTextBrowser(uiParent*,const char* nm="File browser",
				      int maxlns=mUdf(int),
				      bool forceplaintext=true );

    const char*		source() const;
    void		setSource(const char*); 
    void		setMaxLines(int);

    void		backward();
    void		forward();
    void		home();
    void		reload();
    void		scrollToBottom();

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

private:

    uiTextBrowserBody*	body_;
    uiTextBrowserBody&	mkbody(uiParent*,const char*,bool);
};

#endif

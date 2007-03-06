#ifndef uitextedit_h
#define uitextedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.h,v 1.18 2007-03-06 07:38:33 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "undefval.h"

class uiTextEditBody;
class uiTextBrowserBody;
class QTextEdit;

class uiTextEditBase : public uiObject
{
public:

                        uiTextEditBase( uiParent*, const char*, uiObjectBody& );

    void		setText( const char* );
    void		readFromFile( const char* );
    bool		saveToFile( const char* );

    const char*		text() const;
    int			nrLines() const;

    static int          defaultWidth()		    { return defaultWidth_; }
    static void         setDefaultWidth( int w )    { defaultWidth_ = w; }

    static int          defaultHeight()		    { return defaultHeight_; }
    static void         setDefaultHeight( int h )   { defaultHeight_ = h; }

    bool		isModified() const;

protected:

    virtual QTextEdit&	qte()			    = 0;
    const QTextEdit&	qte() const 
			{ return const_cast<uiTextEditBase*>(this)->qte(); }

    static int          defaultWidth_;
    static int          defaultHeight_;
    virtual int		maxLines() const		{ return -1; }

    mutable BufferString result;
};


class uiTextEdit : public uiTextEditBase
{
public:

                        uiTextEdit( uiParent* parnt, 
				    const char* nm="Text editor",
				    bool readonly=false);

    void		append( const char* ); 

protected:

    virtual QTextEdit& qte();

private:

    uiTextEditBody*	body_;
    uiTextEditBody&	mkbody(uiParent*, const char*, bool);
};


class uiTextBrowser : public uiTextEditBase
{
friend class		i_BrowserMessenger;
public:

                        uiTextBrowser(uiParent*, 
				      const char* nm="File browser",
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
    Notifier<uiTextBrowser> goneforwardorback;

    const char* 	lastLink()		{ return lastlink_; }
    Notifier<uiTextBrowser> linkhighlighted;
    Notifier<uiTextBrowser> linkclicked;

protected:

    BufferString	textsrc;
    BufferString	lastlink_;
    bool		cangoforw_;
    bool		cangobackw_;
    bool		forceplaintxt_;
    int			maxlines_;

    virtual int		maxLines() const		{ return maxlines_; }

    virtual QTextEdit& qte();

private:

    uiTextBrowserBody*	body_;
    uiTextBrowserBody&	mkbody(uiParent*, const char*, bool );
};

#endif

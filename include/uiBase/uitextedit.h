#ifndef uitextedit_h
#define uitextedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextedit.h,v 1.16 2007-02-14 12:38:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "undefval.h"

#ifdef USEQT3
# define mQTextEditClss	QTextEdit
#else
# define mQTextEditClss	Q3TextEdit
#endif


class uiTextEditBody;
class uiTextBrowserBody;
class mQTextEditClss;

class uiTextEditBase : public uiObject
{
public:

                        uiTextEditBase( uiParent*, const char*, uiObjectBody& );

    void		setText( const char* );
    void		readFromFile( const char* );
    bool		saveToFile( const char* );

    const char*		text() const;

    static int          defaultWidth()		    { return defaultWidth_; }
    static void         setDefaultWidth( int w )    { defaultWidth_ = w; }

    static int          defaultHeight()		    { return defaultHeight_; }
    static void         setDefaultHeight( int h )   { defaultHeight_ = h; }

    bool		isModified() const;

protected:

    virtual mQTextEditClss&	qte()			    = 0;
    const mQTextEditClss&	qte() const 
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

    virtual mQTextEditClss& qte();

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

    int			nrLines();
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

    virtual mQTextEditClss& qte();

private:

    uiTextBrowserBody*	body_;
    uiTextBrowserBody&	mkbody(uiParent*, const char*, bool );
};

#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "undefval.h"

class uiTextEditBody;
class uiTextBrowserBody;
mFDQtclass(QTextEdit)
class i_ScrollBarMessenger;
namespace File { class Monitor; }

mExpClass(uiBase) uiTextEditBase : public uiObject
{
public:
    void		readFromFile(const char*,int linecutlen=0);
    bool		saveToFile(const char*,int linelen=0,bool newlns=true);

    bool		isEmpty() const override
			{ const auto* t = text(); return !t || !*t; }
    const char*		text() const;
    int			nrLines() const;
    bool		verticalSliderIsDown() const;
			//!<Returns false in absence of the slider

    int			defaultWidth()		  { return defaultwidth_; }
    void		setDefaultWidth( int w )  { defaultwidth_ = w; }

    int			defaultHeight()		  { return defaultheight_; }
    void		setDefaultHeight( int h ) { defaultheight_ = h; }

    bool		isModified() const;
    void		allowTextSelection(bool);

    void		hideFrame();
    void		hideScrollBar(bool vertical);
    void		scrollToBottom();

    Notifier<uiTextEditBase>	textChanged;
    Notifier<uiTextEditBase>	sliderPressed;
    Notifier<uiTextEditBase>	sliderReleased;
    CNotifier<uiTextEditBase,bool>	copyAvailable;

protected:
			uiTextEditBase(uiParent*,const char*,uiObjectBody&);

    mutable BufferString result_;

private:

    virtual mQtclass(QTextEdit&) qte()		= 0;
    const mQtclass(QTextEdit&)   qte() const
			{ return const_cast<uiTextEditBase*>(this)->qte(); }

    virtual int		maxLines() const	{ return -1; }

    int			defaultwidth_;
    int			defaultheight_;

    friend class	i_ScrollBarMessenger;
};



mExpClass(uiBase) uiTextEdit : public uiTextEditBase
{
public:
                        uiTextEdit(uiParent* parnt,const char* nm="Text editor",
				   bool readonly=false);

    void		setEmpty() override		{ setText(0); }
    void		setText(const char* txt)	{ setText(txt,false); }
			//!<Does not trigger notification
    void		setText(const OD::String& txt);
			//!<Does not trigger notification
    void		setText(const uiString&);
			/*!<Will make object read only, does not trigger
			    notification. */
    void		setText(const char*,bool trigger_notif);
    void		append(const char*);

private:

    virtual mQtclass(QTextEdit&)	qte();

    uiTextEditBody*	body_;
    uiTextEditBody&	mkbody(uiParent*,const char*,bool);

};



mExpClass(uiBase) uiTextBrowser : public uiTextEditBase
{ mODTextTranslationClass(uiTextBrowser)
friend class		i_BrowserMessenger;
public:

                        uiTextBrowser(uiParent*,const char* nm="File browser",
				      int maxlns=mUdf(int),
				      bool forceplaintext=true,
				      bool logmode=false);
			~uiTextBrowser();

    void		setEmpty() override		{ setText(0); }
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
    void		showToolTip(const char*);
    void		recordScrollPos();
    void		restoreScrollPos();

    void		updateFilePath(const char* fnm = 0);

    bool		canGoForward()		{ return cangoforw_; }
    bool		canGoBackward()		{ return cangobackw_; }
    const char*		lastLink()		{ return lastlink_; }

    Notifier<uiTextBrowser>	goneForwardOrBack;
    Notifier<uiTextBrowser>	linkHighlighted;
    Notifier<uiTextBrowser>	linkClicked;
    Notifier<uiTextBrowser>	fileReOpened;

private:

    BufferString	textsrc_;
    BufferString	lastlink_;
    bool		cangoforw_;
    bool		cangobackw_;
    bool		forceplaintxt_;
    int			maxlines_;

    virtual int		maxLines() const	{ return maxlines_; }

    void		fileChgCB(CallBacker*);
    void		sliderPressedCB(CallBacker*);
    void		sliderReleasedCB(CallBacker*);
    void		copyAvailableCB(CallBacker*);
    void		enableTailRead(bool yn);

    File::Monitor*	filemon_;
    bool		logviewmode_;
    od_int64		lastlinestartpos_;
    BufferString	lastline_;

    virtual mQtclass(QTextEdit&) qte();

    uiTextBrowserBody*	body_;
    uiTextBrowserBody&	mkbody(uiParent*,const char*,bool);
};

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiparent.h"
#include "uistring.h"

mFDQtclass(QWidget)
class uiDockWin;
class uiGroup;
class uiMainWinBody;
class uiMenu;
class uiMenuBar;
class uiObject;
class uiStatusBar;
class uiToolBar;
class Timer;
class BufferStringSet;

/*!
\brief User interface main window.
*/

mExpClass(uiBase) uiMainWin : public uiParent
{ mODTextTranslationClass(uiMainWin);
public:

    mExpClass(uiBase) Setup
    { mODTextTranslationClass(Setup);
    public:
			Setup( const uiString& capt )
			: caption_(capt)
			, icontxt_(capt)
			, modal_(false)
			, withmenubar_(true)
			, deleteonclose_(true)
			, nrstatusflds_(1)
			{}

	mDefSetupMemb(uiString,caption)
	mDefSetupMemb(uiString,icontxt)
	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,withmenubar)
	mDefSetupMemb(bool,deleteonclose)
	mDefSetupMemb(int,nrstatusflds)
    };

			uiMainWin(uiParent*,const uiMainWin::Setup&);
			uiMainWin(uiParent*,const uiString&
				  caption=uiString::empty(),
				  int nrstatusflds=1,bool withmenubar=true,
				  bool modal=false);
    virtual		~uiMainWin();

    //! Dock Selector
    enum Dock
    {
            Top,        /*!< above the central uiGroup, below the menubar. */
            Bottom,     /*!< below the central uiGroup, above the status bar.*/
            Right,      /*!< to the right of the central uiGroup. */
            Left,       /*!< to the left of the central uiGroup.  */
            Minimized,  /*!< the toolbar is not shown - all handles of
                             minimized toolbars are drawn in one row below
                             the menu bar. */
	    TornOff,	/*!< the dock window floats as its own top level window
			     which always stays on top of the main window. */
	    Unmanaged	/*!< not managed by a uiMainWin */
    };

    uiStatusBar*	statusBar();
    uiMenuBar*		menuBar();

    static uiMainWin*	activeWindow();

    void		setCaption(const uiString&);
    const uiString&	caption(bool unique=false) const;
    void		setIcon(const uiPixmap&);
			//!< Default icon is set in uiMain
    void		setIcon(const char* icon_id);
    void		setIconText(const uiString&);

    bool		showMinMaxButtons(bool yn=true);
    bool		showAlwaysOnTop(bool yn=true);
			/* Return if changed. Will hide the dialog
			   if finalised and already shown */
    void		showAndActivate();
			/* Only for windows already shown before */
    void		setActivateOnFirstShow(bool yn=true);
			/* Activates the window once after its
			   first pops-up */
    void		activate();

    virtual void	show();
    void		close();
    void		raise();
    void		forceClose();

    void		showMaximized();
    void		showMinimized();
    void		showNormal();

    bool		isMaximized() const;
    bool		isMinimized() const;
    bool		isHidden() const;
    bool		isModal() const;

    void		setSensitive(bool yn);

    void		toStatusBar(const uiString&,int fldidx=0,int msecs=-1);

    virtual void	reDraw(bool deep);
    uiGroup*		topGroup();

    void		setShrinkAllowed(bool yn);
    bool		shrinkAllowed();

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose(bool yn);
    void		setDeleteOnClose(bool yn);

    void		removeDockWindow(uiDockWin*);
    void		addDockWindow(uiDockWin&,Dock);
    void		addToolBar(uiToolBar*);
    uiToolBar*		findToolBar(const char*);
    uiToolBar*		removeToolBar(uiToolBar*);
    void		addToolBarBreak();

    uiMenu&		getToolbarsMenu() const;

    const ObjectSet<uiToolBar>& toolBars() const;
    const ObjectSet<uiDockWin>& dockWins() const;

    Notifier<uiMainWin>	windowShown;
    Notifier<uiMainWin> windowHidden;
    Notifier<uiMainWin>	windowClosed;

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(mQtclass(QWidget*));

    enum PopupArea	{ TopLeft, TopRight, BottomLeft, BottomRight,
			  Middle, Auto };
    void		setPopupArea( PopupArea pa )	{ popuparea_ = pa; }
    PopupArea		getPopupArea() const		{ return popuparea_; }
    void		setCornerPos(int x,int y);
			//!Position of top-left corner in screen pixel coords
    uiRect		geometry(bool frame=true) const;

    bool		poppedUp() const;
    bool		resetPopupTimerIfNotPoppedUp();
    bool		finalised() const;
    virtual uiMainWin*	mainwin() { return this; }
    int			getNrWidgets() const		{ return 1; }
    mQtclass(QWidget*)	getWidget(int);
    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const			{ return parent_; }

    enum   ActModalTyp	{ None=0, Main, Message, File, Colour, Font, Unknown };
    static ActModalTyp	activeModalType();
    static uiMainWin*	activeModalWindow();
    static BufferString	activeModalQDlgTitle();
    static BufferString	activeModalQDlgButTxt(int butnr);
    static int		activeModalQDlgRetVal(int butnr);
    static void		closeActiveModalQDlg(int retval);

    static void		getModalSignatures(BufferStringSet&);
    static void		getTopLevelWindows(ObjectSet<uiMainWin>&,
					   bool visibleonly=true);

    static uiString	uniqueWinTitle(const uiString&,
				       mQtclass(QWidget*) forwindow=0,
				       BufferString* addendum = 0);

    void		translateText();


    bool		grab(const char* filenm,int zoom=1,
			     const char* format=0,int quality=-1) const;
			/*!< zoom=0: grab desktop, zoom=1: grab this window,
			     zoom=2: grab active modal window/qdialog,
			     format = .bpm|.jpg|.jpeg|.png|.ppm|.xbm|.xpm
			     format=0: use filenm suffix
			     quality = 0...100: small compressed to large
			     uncompressed file, quality=-1: use default    */
    static bool		grabScreen(const char* filenm,const char* format=0,
				   int quality=-1,int screen=0);

    void		activateInGUIThread(const CallBack&,
					    bool busywait=true);
    void		saveImage(const char* fnm,int w,int h,int res);
    void		saveAsPDF(const char* fnm,int w,int h,int res)
			{ saveAsPDF_PS( fnm, true, w, h, res ); }
    void		saveAsPS(const char* fnm,int w,int h,int res)
			{ saveAsPDF_PS( fnm, false, w, h, res ); }
    Notifier<uiMainWin> activatedone;
    Notifier<uiMainWin> ctrlCPressed;
    Notifier<uiMainWin> afterPopup;
    void		copyToClipBoard();

    Notifier<uiMainWin> runScriptRequest;
    void		runScript(const char* filename);
    const char*		getScriptToRun() const;

protected:

    friend class	uiMainWinBody;

    virtual bool	closeOK()	{ return true; }

			uiMainWin(const uiString&,uiParent*);
    void		finishConstruction();
    uiObject*		mainobject();

    void		saveSettings();
    void		readSettings();
    void		saveAsPDF_PS(const char* fnm,bool aspdf,int w,int h,
				     int res);

    void		copyToClipBoardCB(CallBacker*);
    void		aftPopupCB(CallBacker*);
    void		languageChangeCB(CallBacker*);
    void		setForceFinalise(bool);

    uiMainWinBody*	body_			= nullptr;
    uiParent*		parent_;
    Timer*		afterpopuptimer_	= nullptr;

    PopupArea		popuparea_		= Auto;

    void		updateCaption();
    uiString		caption_;
    uiString		uniquecaption_;
    int			languagechangecount_;

    BufferString	scripttorun_;

    static bool		haveModalWindows();

private:

    bool		doSetWindowFlags(od_uint32 qtwinflag,bool set);

public:
			// Not for casual use
    static void		programActiveWindow(uiMainWin*);
    static uiMainWin*	programmedActiveWindow();
    static void		setActivateBehaviour(OD::WindowActivationBehavior);
    static OD::WindowActivationBehavior getActivateBehaviour();

};


template <class T>
void closeAndZeroPtr( T*& ptr )
{
    auto* uimw = dCast( uiMainWin*, ptr );
    if ( uimw ) uimw->forceClose();
    ptr = nullptr;
}

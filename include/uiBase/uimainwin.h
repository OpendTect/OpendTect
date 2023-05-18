#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "odcommonenums.h"
#include "uiparent.h"
#include "uistring.h"

mFDQtclass(QWidget)
mFDQtclass(QScreen)
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
friend class uiMainWinBody;
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
			~Setup()
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
				  caption=uiString::emptyString(),
				  int nrstatusflds=1,bool withmenubar=true,
				  bool modal=false);
    virtual		~uiMainWin();
			mOD_DisableCopy(uiMainWin)

    //! Dock Selector
    enum Dock
    {
	Top,	    /*!< above the central uiGroup, below the menubar. */
	Bottom,     /*!< below the central uiGroup, above the status bar.*/
	Right,	    /*!< to the right of the central uiGroup. */
	Left,	    /*!< to the left of the central uiGroup.  */
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
    void		setIconText(const uiString&);

    bool		showMinMaxButtons(bool yn=true);
    bool		showAlwaysOnTop(bool yn=true);
			/* Return if changed. Will hide the dialog
			   if finalized and already shown */
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

    void		reDraw(bool deep) override;
    uiGroup*		topGroup();

    void		setShrinkAllowed(bool yn);
    bool		shrinkAllowed();

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose(bool yn);
    void		setDeleteOnClose(bool yn);
    bool		isDeleteOnClose() const;

    void		removeDockWindow(uiDockWin*);
    void		addDockWindow(uiDockWin&,Dock);
    void		addToolBar(uiToolBar*);
    uiToolBar*		findToolBar(const char*);
    uiToolBar*		removeToolBar(uiToolBar*);
    void		addToolBarBreak();

    uiMenu&		getToolbarsMenu() const;
    void		restoreDefaultState();

    const ObjectSet<uiToolBar>& toolBars() const;
    const ObjectSet<uiDockWin>& dockWins() const;

    Notifier<uiMainWin>	windowShown;
    Notifier<uiMainWin> windowHidden;
    Notifier<uiMainWin>	windowClosed;
			//!< triggered when window exits

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(mQtclass(QWidget*) mwimpl);

    enum PopupArea	{ TopLeft, TopRight, BottomLeft, BottomRight,
			  Middle, Auto };
    void		setPopupArea( PopupArea pa )	{ popuparea_ = pa; }
    PopupArea		getPopupArea() const		{ return popuparea_; }
    void		setCornerPos(int x,int y);
			//!Position of top-left corner in screen pixel coords
    uiRect		geometry(bool frame=true) const;

    bool		poppedUp() const;
    bool		touch(); //!< resets pop-up timer if !poppedUp yet
    bool		finalized() const;
    uiMainWin*		mainwin() override		{ return this; }
    mQtclass(QWidget*)	qWidget() const;
    mQtclass(QScreen*)	qScreen() const;
    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const			{ return parent_; }

    enum   ActModalTyp	{ None=0, Main, Message, File, Color, Font, Unknown };
    static ActModalTyp	activeModalType();
    static uiMainWin*	activeModalWindow();
    static const char*	activeModalQDlgTitle();
    static const char*	activeModalQDlgButTxt(int butnr);
    static int		activeModalQDlgRetVal(int butnr);
    static void		closeActiveModalQDlg(int retval);

    static void		getModalSignatures(BufferStringSet&);
    static void		getTopLevelWindows(ObjectSet<uiMainWin>&,
					   bool visibleonly=true);

    static uiString	uniqueWinTitle(const uiString&,
				       mQtclass(QWidget*) forwindow=0,
				       BufferString* addendum = 0);

    void		translateText() override;


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
    static bool		grabScreen(const char* filenm,const char* format=0,
				   int quality=-1,
				   mQtclass(QScreen*) qscreen=nullptr);

    void		activateInGUIThread(const CallBack&,
					    bool busywait=true);
    void		saveImage(const char* fnm,int w,int h,int res);
    void		saveImage(const char* fnm, const char* fmt=nullptr,
				  int quality=-1 );
    void		saveAsPDF(const char* fnm,int w,int h,int res);
    mDeprecatedObs
    void		saveAsPS(const char* fnm,int w,int h,int res)
			{ saveAsPDF( fnm, w, h, res ); }
    Notifier<uiMainWin> activatedone;
    Notifier<uiMainWin> ctrlCPressed;
    Notifier<uiMainWin> afterPopup;
    void		copyToClipBoard();

    Notifier<uiMainWin> runScriptRequest;
    void		runScript(const char* filename);
    const char*		getScriptToRun() const;

    mDeprecated("Use finalized()")
    bool		finalised() const	{ return finalized(); }

protected:

    virtual bool	closeOK()	{return true;}//!< confirm window close

			uiMainWin(uiString,uiParent*);
    uiObject*		mainobject() override;

    void		saveSettings();
    void		readSettings();

    void		saveAsPDF_PS(const char* fnm,int w,int h,int res);

    void		copyToClipBoardCB(CallBacker*);
    void		aftPopupCB(CallBacker*);
    void		languageChangeCB(CallBacker*);
    void		setForceFinalize(bool);
    mDeprecated("Use setForceFinalize()")
    void		setForceFinalise( bool yn )
			{ setForceFinalize( yn ); }
    void		setContentsMargins(int l,int t,int r,int b);

    uiMainWinBody*	body_;
    uiParent*		parent_;
    Timer*		afterpopuptimer_;

    PopupArea		popuparea_;

    void		updateCaption();
    uiString		caption_;
    uiString		uniquecaption_;
    int			languagechangecount_;

    BufferString	scripttorun_;

private:

    bool		doSetWindowFlags(od_uint32 qtwinflag,bool set);

public:
			// Not for casual use
    static void		programActiveWindow(uiMainWin*);
    static uiMainWin*	programmedActiveWindow();
    static void		setActivateBehavior(OD::WindowActivationBehavior);
    static OD::WindowActivationBehavior getActivateBehavior();

};


template <class T>
void closeAndNullPtr( T*& ptr )
{
    if ( !ptr )
	return;

    auto* uimw = dCast( uiMainWin*, ptr );
    if ( !uimw )
	return;

    const bool explicitdelete = !uimw->isModal() || uimw->isDeleteOnClose();
    auto* winptr = uimw;
    uimw->forceClose();
    if ( explicitdelete )
	delete winptr;

    ptr = nullptr;
}

template <class T>
mDeprecated("Use closeAndNullPtr")
void closeAndZeroPtr( T*& ptr )
{ closeAndNullPtr( ptr); }

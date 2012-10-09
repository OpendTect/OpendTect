#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiparent.h"
#include "mousecursor.h"

class uiDockWin;
class uiGroup;
class uiMainWinBody;
class uiMenuBar;
class uiObject;
class uiPopupMenu;
class uiStatusBar;
class uiToolBar;
class QWidget;
class BufferStringSet;

mClass uiMainWin : public uiParent
{
friend class uiMainWinBody;
public:
    mClass Setup
    {
    public:
			Setup( const char* capt )
			: caption_(capt)
			, icontxt_(capt)
			, modal_(false)
			, withmenubar_(true)
			, deleteonclose_(true)
			, nrstatusflds_(1)
			{}

	mDefSetupMemb(BufferString,caption)
	mDefSetupMemb(BufferString,icontxt)
	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,withmenubar)
	mDefSetupMemb(bool,deleteonclose)
	mDefSetupMemb(int,nrstatusflds)
    };

    			uiMainWin(uiParent*,const uiMainWin::Setup&);
			uiMainWin(uiParent*,const char* nm="uiMainWin",
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

    uiStatusBar* 	statusBar();
    uiMenuBar* 		menuBar();

    static uiMainWin*	activeWindow();

    void		setCaption(const char*);
    const char*		caption(bool unique=false) const;
    void		setIcon(const ioPixmap&);
			//!< Default icon is set in uiMain
    void		setIconText(const char*);

    virtual void	show();
    void                close();
    void		raise();

    void		showMaximized();
    void		showMinimized();
    void		showNormal();

    bool		isMaximized() const;
    bool		isMinimized() const;
    bool		isHidden() const;
    bool		isModal() const;

    void		setSensitive(bool yn);

    void		toStatusBar(const char*,int fldidx=0,int msecs=-1);

    virtual void	reDraw(bool deep);
    uiGroup* 		topGroup();

    void		setShrinkAllowed(bool yn);
    bool		shrinkAllowed();

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose(bool yn);
    void		setDeleteOnClose(bool yn);

    void		removeDockWindow(uiDockWin*);
    void		addDockWindow(uiDockWin&,Dock);
    void		addToolBar(uiToolBar*);
    void		removeToolBar(uiToolBar*);
    void		addToolBarBreak();

    uiPopupMenu& 	getToolbarsMenu() const;
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const;
    const ObjectSet<uiDockWin>& dockWins() const;

    Notifier<uiMainWin>	windowClosed;
    			//!< triggered when window exits

    static void		provideHelp(const char* winid=0);
    static void		showCredits(const char* winid=0);

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(QWidget* mwimpl);

    enum PopupArea	{ TopLeft, TopRight, BottomLeft, BottomRight,
			  Middle, Auto };
    void		setPopupArea( PopupArea pa )	{ popuparea_ = pa; }
    PopupArea		getPopupArea() const		{ return popuparea_; }
    void		setCornerPos(int x,int y);
    			//!Position of top-left corner in screen pixel coords
    uiRect		geometry(bool frame=true) const;

    bool		poppedUp() const;
    bool		touch(); //!< resets pop-up timer if !poppedUp yet
    bool		finalised() const;
    virtual uiMainWin*	mainwin()			{ return this; }
    QWidget*		qWidget() const;
    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const			{ return parent_; }

    enum   ActModalTyp	{ None=0, Main, Message, File, Colour, Font, Unknown };
    static ActModalTyp	activeModalType();
    static uiMainWin*	activeModalWindow();
    static const char*	activeModalQDlgTitle();
    static const char*	activeModalQDlgButTxt(int butnr);
    static int		activeModalQDlgRetVal(int butnr);
    static void		closeActiveModalQDlg(int retval);

    static void		getModalSignatures(BufferStringSet&);
    static void		getTopLevelWindows(ObjectSet<uiMainWin>&,
					   bool visibleonly=true);
    static const char*	uniqueWinTitle(const char* txt,QWidget* forwindow=0);

    void		translate();


    bool		grab(const char* filenm,int zoom=1,
			     const char* format=0,int quality=-1) const;
			/*!< zoom=0: grab desktop, zoom=1: grab this window,
			     zoom=2: grab active modal window/qdialog,
			     format = .bpm|.jpg|.jpeg|.png|.ppm|.xbm|.xpm
			     format=0: use filenm suffix
			     quality = 0...100: small compressed to large
			     uncompressed file, quality=-1: use default    */

    void		activateInGUIThread(const CallBack&,bool busywait=true);
    Notifier<uiMainWin> activatedone;
    Notifier<uiMainWin> ctrlCPressed;

protected:

    virtual bool	closeOK() 	{return true;}//!< confirm window close

			uiMainWin(const char*,uiParent*);
    uiObject*		mainobject();

    void		saveSettings();
    void		readSettings();

    void		copyToClipBoard(CallBacker*);

    uiMainWinBody*	body_;
    uiParent*		parent_;

    PopupArea		popuparea_;

    BufferString	caption_;

public:	
			// Not for casual use
    static void		programActiveWindow(uiMainWin*);
    static uiMainWin*	programmedActiveWindow();
};

#endif

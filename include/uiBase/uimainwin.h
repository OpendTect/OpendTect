#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.52 2007-09-18 14:25:24 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uihandle.h"

class uiDockWin;
class uiGroup;
class uiMainWinBody;
class uiMenuBar;
class uiObject;
class uiPopupMenu;
class uiStatusBar;
class uiToolBar;
class QWidget;

class uiMainWin : public uiParent
{
friend class uiMainWinBody;
public:
			/*!
			    nrStatusFlds == 0	: no statysbar
			    nrStatusFlds < 0	: creates empty statusbar.
				Add status fields yourself in that case.

			    if no parent, then modal is ignored
			     (always non-modal).
			*/
			uiMainWin( uiParent* parnt=0, 
				   const char* nm="uiMainWin",
				   int nrStatusFlds = 1, 
				   bool wantMenuBar = true,
				   bool modal=false );

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

    enum ActModalTyp	{ None=0, Main, Message, File, Colour, Font, Unknown };
    static ActModalTyp	activeModalType();
    static uiMainWin*	activeModalWindow();

    static char*	activeModalQDlgButTxt(int butnr);
    static int		activeModalQDlgRetVal(int butnr);

    static void		getTopLevelWindows( ObjectSet<uiMainWin>& );

    void		setCaption( const char* txt );
    void		setIcon(const char* img[],const char* icntxt); //!< XPM
    virtual void        show();
    void                close();

    void		activateClose();    //! Force activation in GUI thread
    void		activateQDlg(int retval);
    Notifier<uiMainWin> activatedone;

    bool		isHidden() const;
    bool		isModal() const;

    void		setSensitive(bool yn=true);

    void		toStatusBar(const char*, int fldidx=0, int msecs=-1 );

    virtual void	reDraw(bool deep);
    uiGroup* 		topGroup();

    void		setShrinkAllowed( bool yn=true );
    bool		shrinkAllowed();

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose( bool yn );

    void		moveDockWindow(uiDockWin&,Dock d=Top,int index=-1);
    void		removeDockWindow(uiDockWin*);
    void		addDockWindow(uiDockWin&,Dock);
    void		addToolBar(uiToolBar*);
    void		removeToolBar(uiToolBar*);
    void		addToolBarBreak();
    Notifier<uiMainWin>	nrToolBarsChange;
    Notifier<uiMainWin>	toolBarsDispChg;

    Notifier<uiMainWin>	windowClosed;
    			//!< triggered when window exits

    static void		provideHelp(const char* winid=0);

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(QWidget* mwimpl);

    bool		poppedUp() const;
    bool		touch(); //!< resets pop-up timer if !poppedUp yet
    bool		finalised() const;
    virtual uiMainWin*	mainwin()			{ return this; }
    QWidget*		qWidget() const;
    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const			{ return parent_; }

protected:

    virtual bool	closeOK() 	{return true;}//!< confirm window close

			uiMainWin(const char*,uiParent*);
    uiObject*		mainobject();

    uiMainWinBody*	body_;
    uiParent*		parent_;
};

#endif

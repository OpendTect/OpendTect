#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.35 2004-04-29 12:33:29 arend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiMainWinBody;
class uiStatusBar;
class uiToolBar;
class uiMenuBar;
class uiObject;
class uiGroup;
class QWidget;
class uiDockWin;

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
				   bool wantToolBar = false,
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
    uiToolBar* 		toolBar();
    uiToolBar* 		newToolBar(const char* nm="ToolBar");

    static uiMainWin*	activeWindow();

    void		setCaption( const char* txt );
    void		setIcon(const char* img[],const char* icntxt); //!< XPM
    void                show();
    void                close();
    void		toStatusBar(const char*, int fldidx=0, int msecs=-1 );

    virtual void	reDraw(bool deep);
    uiGroup* 		topGroup();

    void		setShrinkAllowed( bool yn=true );
    bool		shrinkAllowed();

			//! automatically set by uiMain::setTopLevel
    void		setExitAppOnClose( bool yn );

    void		moveDockWindow(uiDockWin&,Dock d=Top,int index=-1);
    void		removeDockWindow(uiDockWin*);

    Notifier<uiMainWin>	finaliseStart;
    			//!< triggered when about to start finalising
    Notifier<uiMainWin>	finaliseDone;
    			//!< triggered when finalising finished
    Notifier<uiMainWin>	windowClosed;
    			//!< triggered when window exits

    static void		provideHelp(const char* winid=0);

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(QWidget* mwimpl);


    bool		poppedUp() const;
    bool		touch(); //!< resets pop-up timer if !poppedUp yet
    bool		finalised() const;
    virtual uiMainWin*	mainwin()				{ return this; }

protected:

    virtual bool	closeOK() 	{return true;}//!< confirm window close

			uiMainWin( const char* );
    uiObject*		mainobject();

    uiMainWinBody*	body_;
};

#endif

#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.h,v 1.6 2002-03-21 16:21:31 nanne Exp $
________________________________________________________________________

-*/

#include <uiparent.h>

class ioPixmap;
class uiToolBarBody;
class QMainWindow;
class QToolBar;

class uiToolBar : public uiParent
{
public:
    //! ToolBar Dock Identifier
    /*
	Toolbars can be created on docks,
    */
    enum ToolBarDock 
    { 
	    Top,	/*!< above the central uiGroup, below the menubar. */
	    Bottom,	/*!< below the central uiGroup, above the status bar.*/
	    Right,	/*!< to the right of the central uiGroup. */
	    Left,	/*!< to the left of the central uiGroup.  */
	    Minimized	/*!< the toolbar is not shown - all handles of
			     minimized toolbars are drawn in one row below
			     the menu bar. */
    };

    static uiToolBar*	getNew( QMainWindow& main, const char* nm="uiToolBar",
				ToolBarDock d=Top, bool newline=false );
protected:
			uiToolBar( const char* nm, QToolBar& );
public:

    void 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton" );

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

    void		addSeparator();
    void		setStretchableWidget(uiObject*);
    void		setMovingEnabled(bool);
    bool		isMovingEnabled();

protected:

    uiToolBarBody*	body_;
    uiToolBarBody&	mkbody(const char*, QToolBar&);

};

#endif

#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.h,v 1.1 2001-05-30 16:36:14 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <sets.h>

class QToolBar;
class QMainWindow;
class ioPixmap;

class i_QToolButReceiver;

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

    static uiToolBar*	getNew( const char* nm="uiToolBar", ToolBarDock d=Top,
			        bool newline=false, uiMainWin* main=0 );

			uiToolBar( const char* nm, QMainWindow*, ToolBarDock,
				   bool newline=false );

			~uiToolBar();

    inline QToolBar*	mQtThing()	{ return qbar; }

    void 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton" );

protected:

    QToolBar*		qbar;

    virtual const QWidget* qWidget_() const;

    int			qdock(ToolBarDock);

private:
    ObjectSet<i_QToolButReceiver> receivers; // for deleting
};

#endif

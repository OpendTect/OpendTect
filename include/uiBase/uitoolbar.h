#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.h,v 1.17 2004-11-09 10:25:59 nanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"

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

    			uiToolBar(uiParent*,const char* nm,
				  ToolBarDock d=Top,bool newline=false);
			~uiToolBar();

    int 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton",
				   bool toggle=false );

    void		setLabel(const char*);

    void		setToolTip(int,const char*);
    void		turnOn( int idx, bool yn );
    			/*!< Does only work on toggle-buttons */
    bool		isOn( int idx ) const;
    			/*!< Does only work on toggle-buttons */
    void		setSensitive( int idx, bool yn );
    			/*!< Does only work on toggle-buttons */
    void		setSensitive( bool yn );
    			/*!< Works on complete toolbar */

    virtual void	display(bool yn=true, bool s=false,bool m=false);
			/*!< you must call this after all buttons are added
			     s and m are not used.
			*/

    void		addSeparator();
    void		setStretchableWidget(uiObject*);


    void		setMovingEnabled(bool);
    bool		isMovingEnabled() const;

    void		setCloseMode(int);
    			/*!< 0: Never; 1: Docked; 2: Undocked; 3: Always */
    int			closeMode() const;
    void		setHorizontallyStretchable(bool yn=true);
    bool		isHorizontallyStretchable() const;
    void		setVerticallyStretchable(bool yn=true);
    bool		isVerticallyStretchable() const;
    void		setResizeEnabled(bool yn=true);
    bool		isResizeEnabled() const;

    void		dock();
    void		undock();
    void		setNewLine(bool yn=true);

protected:

    QToolBar*		qtoolbar;
    uiToolBarBody*	body_;
    uiToolBarBody&	mkbody(const char*,QToolBar&);

};

#endif

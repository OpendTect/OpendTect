#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.h,v 1.29 2007-10-03 08:28:28 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiparent.h"

class ioPixmap;
class uiObject;
class uiToolBarBody;
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

    int 		addButton(const char*,const CallBack&,
				  const char* tooltip,bool toggle=false);
    int 		addButton(const ioPixmap&,const CallBack&,
				  const char* tooltip,bool toggle=false);
    void		addObject(uiObject*);

    void		setLabel(const char*);

    void		setPixmap(int,const char*);
    void		setPixmap(int,const ioPixmap&);
    void		setToolTip(int,const char*);
    void		setShortcut(int,const char*);
    void		turnOn(int idx,bool yn);
    			/*!< Does only work on toggle-buttons */
    bool		isOn(int idx) const;
    			/*!< Does only work on toggle-buttons */
    void		setSensitive(int idx,bool yn);
    			/*!< Does only work on buttons */
    void		setSensitive(bool yn);
    			/*!< Works on complete toolbar */

    virtual void	display(bool yn=true,bool s=false,bool m=false);
			/*!< you must call this after all buttons are added
			     s and m are not used.
			*/
    bool		isHidden() const;
    bool		isVisible() const;

    void		addSeparator();

    void		setStretchableWidget(uiObject*);
#ifdef USEQT3
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
#endif
    bool		isShown() const;

    void		reLoadPixMaps();
    void		clear();

    const ObjectSet<uiObject>& 		objectList() const;

    static ObjectSet<uiToolBar>&	toolBars();
    QToolBar*		qwidget()	{ return qtoolbar_; }

protected:

    QToolBar*		qtoolbar_;
    uiToolBarBody*	body_;
    uiToolBarBody&	mkbody(const char*,QToolBar&);

    uiParent*		parent_;

};

#endif

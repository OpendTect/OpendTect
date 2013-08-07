#ifndef uitoolbar_h
#define uitoolbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uiaction.h"
#include "uiparent.h"

mFDQtclass(QAction)
mFDQtclass(QToolBar)

class ioPixmap;
class MenuItem;
class uiObject;
class uiToolButton;
class uiToolButtonSetup;
class i_ToolBarMessenger;
class uiButton;


mExpClass(uiBase) uiToolBar : public uiActionContainer, public uiParent
{
friend class i_ToolBarMessenger;
public:
    //! ToolBar Dock Identifier
    /*
	Toolbars can be created on docks,
    */
    enum ToolBarArea 
    { 
	    Left=0x1,	//!< To the left of the central uiGroup.
	    Right=0x2,	//!< To the right of the central uiGroup.
	    Top=0x4,	//!< Above the central uiGroup, below the menubar.
	    Bottom=0x8,	//!< Below the central uiGroup, above the status bar.
	    None=0,	//!< No toolbar area
	    All=0xf	//!< All areas.
    };

    			uiToolBar(uiParent*,const char* nm,
				  ToolBarArea d=Top,bool newline=false);
			~uiToolBar();

    int 		addButton(const uiToolButtonSetup&);
    int 		addButton(const char* fnm,const char* tooltip,
	    			  const CallBack& =CallBack(),
				  bool toggle=false);
    int			addButton(const MenuItem&);
    void		addObject(uiObject*);

    void		setLabel(const char*);

    void		setToggle(int id, bool);
    void		setIcon(int id,const char*);
    void		setIcon(int id,const ioPixmap&);
    void		setToolTip(int id,const char*);
    void		setShortcut(int id,const char*);
    void		turnOn(int id,bool yn);
    			/*!< Does only work on toggle-buttons */
    bool		isOn(int id) const;
    			/*!< Does only work on toggle-buttons */
    void		setSensitive(int id,bool yn);
    			/*!< Does only work on buttons */
    void		setSensitive(bool yn);
    			/*!< Works on complete toolbar */
    bool		isSensitive() const;

    void		setButtonMenu(int,uiMenu*);
    			//!<Menu will be owned by uiToolButton

    virtual void	display(bool yn=true,bool s=false,bool m=false);
			/*!< you must call this after all buttons are added
			     s and m are not used. */

    void		setToolBarMenuAction(uiAction*);
    			/*!The actions will be checked/unchecked
			   as toolbar is displayed/hidden */
			  
    void		translate();

    bool		isHidden() const;
    bool		isVisible() const;

    void		addSeparator() { insertSeparator(); }
    
    void		clear();

    static ToolBarArea	pluginArea()		{ return uiToolBar::Right; }
    ToolBarArea		prefArea() const	{ return tbarea_; }
    mQtclass(QToolBar*)	qwidget()		{ return qtoolbar_; }

    static ObjectSet<uiToolBar>&	toolBars();

    CNotifier<uiToolBar,int>		buttonClicked;

protected:
	    
    void			doInsertMenu(mQtclass(QMenu)*,
					     mQtclass(QAction)* before);
    void			doInsertAction(mQtclass(QAction)*,
					       mQtclass(QAction)* before);
    void			doInsertSeparator(mQtclass(QAction)* before);
    void			doRemoveAction(mQtclass(QAction)*);
    void			doClear();

    uiAction*			toolbarmenuaction_;

    mQtclass(QToolBar*)		qtoolbar_;
    i_ToolBarMessenger*		msgr_;

    ToolBarArea			tbarea_;
    ObjectSet<uiObject>		addedobjects_;

    int				getButtonID(mQtclass(QAction*));
    
public:
    			//!CmdDriver functionality, not for casual use
    void		getEntityList(ObjectSet<const CallBacker>&) const;

    void		addButton(uiButton*);
			//!<Legacy, use addObject instead
};

#endif


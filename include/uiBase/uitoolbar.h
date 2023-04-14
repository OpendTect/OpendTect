#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uiaction.h"
#include "uiparent.h"
#include "uitoolbutton.h"

mFDQtclass(QAction)
mFDQtclass(QToolBar)

class MenuItem;
class uiButton;
class uiIcon;
class uiObject;
class i_ToolBarMessenger;


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

			uiToolBar(uiParent*,const uiString& nm,
				  ToolBarArea d=Top,bool newline=false);
			uiToolBar(uiParent*,const char* nm,
				  ToolBarArea d=Top,bool newline=false);
			~uiToolBar();

    uiParent*		parent()			{ return parent_; }

    int			addButton(const uiToolButtonSetup&);
    int			addButton(const char* fnm,const uiString& tooltip,
				  const CallBack& =CallBack(),
				  bool toggle=false,int id=-1);
    int			addButton(const MenuItem&);
    void		addObject(uiObject*);
    void		removeObject(uiObject*);
    bool		hasObject(const uiObject*) const;

    void		setLabel(const uiString&);
    uiString		getDispNm() const;

    void		setToggle(int id, bool);
    void		setIcon(int id,const char*);
    void		setIcon(int id,const uiIcon&);
    void		setToolTip(int id,const uiString&);
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
    OD::Orientation	getOrientation() const;

    void		setButtonMenu(int,uiMenu*,
			 uiToolButton::PopupMode=uiToolButton::MenuButtonPopup);
			//!<Menu will be owned by uiToolButton

    void		display(bool yn=true,bool s=false,
				bool m=false) override;
			/*!< you must call this after all buttons are added
			     s and m are not used. */

    void		setToolBarMenuAction(uiAction*);
			/*!The actions will be checked/unchecked
			   as toolbar is displayed/hidden */

    void		translateText() override;

    bool		isHidden() const;
    bool		isVisible() const;

    void		addSeparator() { insertSeparator(); }
    void		setTabOrder(uiObject*,uiObject*);

    void		clear();

    static ToolBarArea	pluginArea()		{ return uiToolBar::Right; }
    ToolBarArea		prefArea() const	{ return tbarea_; }
    mQtclass(QToolBar*)	qwidget()		{ return qtoolbar_; }

    static ObjectSet<uiToolBar>& toolBars();

    CNotifier<uiToolBar,int>	buttonClicked;
    Notifier<uiToolBar>		orientationChanged;

protected:

    void			doInsertMenu(mQtclass(QMenu)*,
					mQtclass(QAction)* before) override;
    void			doInsertAction(mQtclass(QAction)*,
					mQtclass(QAction)* before) override;
    void			doInsertSeparator(
					mQtclass(QAction)* before) override;
    void			doRemoveAction(mQtclass(QAction)*) override;
    void			doClear() override;

    mDeprecated("Use handleFinalize")
    void			handleFinalise( bool pre )
				{ handleFinalize( pre ); }
    void			handleFinalize(bool pre);

    uiAction*			toolbarmenuaction_;

    mQtclass(QToolBar*)		qtoolbar_;
    i_ToolBarMessenger*		msgr_;

    ToolBarArea			tbarea_;
    ObjectSet<uiObject>		addedobjects_;

    uiString			label_;
    int				getButtonID(mQtclass(QAction*));

    uiParent*			parent_;
    friend class		uiMainWinBody;

public:
			//!CmdDriver functionality, not for casual use
    void		getEntityList(ObjectSet<const CallBacker>&) const;

    mDeprecated("Use addObject instead")
    void		addButton(uiButton*);

};

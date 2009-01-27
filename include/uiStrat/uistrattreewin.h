#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.21 2009-01-27 11:44:12 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiListBox;
class uiListViewItem;
class uiMenuItem;
class uiStratLevelDlg;
class uiStratLinkLvlUnitDlg;
class uiStratMgr;
class uiStratRefTree;
class uiStratTreeWin;
class uiToolBar;
class uiToolButton;
namespace Strat{ class Level; }

mGlobal const uiStratTreeWin& StratTWin();

/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

mClass uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

    void		popUp() const;

    mutable Notifier<uiStratTreeWin>	levelCreated;
    mutable Notifier<uiStratTreeWin>	levelChanged;
    mutable Notifier<uiStratTreeWin>	levelRemoved;
    mutable Notifier<uiStratTreeWin>	unitCreated;
    mutable Notifier<uiStratTreeWin>	unitChanged;
    mutable Notifier<uiStratTreeWin>	unitRemoved;
    mutable Notifier<uiStratTreeWin>	lithCreated;
    mutable Notifier<uiStratTreeWin>	lithChanged;
    mutable Notifier<uiStratTreeWin>	lithRemoved;
    mutable Notifier<uiStratTreeWin>	newUnitSelected;
    mutable Notifier<uiStratTreeWin>	newLevelSelected;

protected:

    uiStratRefTree*		uitree_;
    uiListBox*			lvllistfld_;
    uiStratLinkLvlUnitDlg*	linkunlvldlg_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			openmnuitem_;
    uiMenuItem*			resetmnuitem_;
    uiToolBar*			tb_;
    uiToolButton*		colexpbut_;
    uiToolButton*		lockbut_;
    uiToolButton*		openbut_;
    uiToolButton*		savebut_;
    uiToolButton*		moveunitupbut_;
    uiToolButton*		moveunitdownbut_;

    void			createMenu();
    void			createToolBar();
    void			createGroups();

    void			fillLvlList();
    void			updateLvlList(bool);
    void			editLevel(bool);
    void			fillInLvlPars(Strat::Level*,
	    				      const uiStratLevelDlg&,bool);

    void			editCB(CallBacker*);
    void			openCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			selLvlChgCB(CallBacker*);
    void			rClickLvlCB(CallBacker*);
    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);
    void			unitRenamedCB(CallBacker*);
    void			moveUnitCB(CallBacker*);
    void			shutdownCB(CallBacker*);

    uiStratMgr*			uistratmgr_;

    bool			needsave_;
    virtual bool		closeOK();

private:

    friend const uiStratTreeWin& StratTWin();
};


#endif

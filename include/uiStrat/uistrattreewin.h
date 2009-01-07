#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.15 2009-01-07 15:58:39 cvsbert Exp $
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
namespace Strat{ class Level; }

const uiStratTreeWin& StratTWin();

/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

class uiStratTreeWin : public uiMainWin
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

    void			createMenus();
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

    uiStratMgr*			uistratmgr_;

private:

    friend const uiStratTreeWin& StratTWin();
};


#endif

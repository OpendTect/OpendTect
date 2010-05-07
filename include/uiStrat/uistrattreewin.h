#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.27 2010-05-07 12:50:46 cvsbruno Exp $
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
class uiStratDisplay;
class uiToolBar;
class uiToolButton;
namespace Strat{ class Level; }

mGlobal const uiStratTreeWin& StratTWin();
mGlobal uiStratTreeWin& StratTreeWin();

/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

mClass uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

    void		popUp() const;
    virtual bool	closeOK();
    
    mutable Notifier<uiStratTreeWin>	levelCreated;
    mutable Notifier<uiStratTreeWin>	levelChanged;
    mutable Notifier<uiStratTreeWin>	levelRemoved;
    mutable Notifier<uiStratTreeWin>	newUnitSelected;
    mutable Notifier<uiStratTreeWin>	newLevelSelected;

#define mCreateCoupledNotifCB(nm) \
public: \
    mutable Notifier<uiStratTreeWin> nm;\
protected: \
    void nm##CB(CallBacker*);

    mCreateCoupledNotifCB( unitCreated )
    mCreateCoupledNotifCB( unitChanged )
    mCreateCoupledNotifCB( unitRemoved )
    mCreateCoupledNotifCB( lithCreated )
    mCreateCoupledNotifCB( lithChanged )
    mCreateCoupledNotifCB( lithRemoved )

protected:

    uiStratMgr&			uistratmgr_;
    uiStratRefTree*		uitree_;
    uiListBox*			lvllistfld_;
    uiStratDisplay*		uistratdisp_;
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
    uiToolButton*		switchviewbut_;
    bool			needsave_;
    bool			needcloseok_;
    bool			istreedisp_;

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
    void			switchViewCB(CallBacker*);
    void			unitSelCB(CallBacker*);
    void			unitRenamedCB(CallBacker*);
    void			moveUnitCB(CallBacker*);
    void			forceCloseCB(CallBacker*);
    void			helpCB(CallBacker*);

private:

    friend const uiStratTreeWin& StratTWin();
};


#endif

#ifndef uistrattreewin_h
#define uistrattreewin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.31 2010-08-04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiListBox;
class uiListViewItem;
class uiMenuItem;
class uiStratLevelDlg;
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
    
    const uiStratMgr&	mgr() const	{ return uistratmgr_; }
    
    void                addLevels(const BufferStringSet&,const TypeSet<Color>&);

    mutable Notifier<uiStratTreeWin>    levelCreated;
    mutable Notifier<uiStratTreeWin>    levelChanged;
    mutable Notifier<uiStratTreeWin>    levelRemoved;
    mutable Notifier<uiStratTreeWin>    newLevelSelected;
    mutable Notifier<uiStratTreeWin>	newUnitSelected;

    
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
    uiStratDisplay*		uistratdisp_;
    uiListBox*                  lvllistfld_;
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

    void                        fillLvlList();
    void                        updateLvlList(bool);
    void                        editLevel(bool);
    void                        fillInLvlPars(Strat::Level*,
					    const uiStratLevelDlg&,bool);

    void			editCB(CallBacker*);
    void			openCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveCB(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
    void			saveAsCB(CallBacker*);
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

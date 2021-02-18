#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uimainwin.h"
#include "manobjectset.h"
#include "uitoolbutton.h"

class uiAction;
class uiStratLvlList;
class uiStratRefTree;
class uiStratTreeWin;
class uiStratDisplay;
class uiToolBar;
class uiToolButton;

namespace Strat { class RepositoryAccess; }

mGlobal(uiStrat) const uiStratTreeWin& StratTWin();
mGlobal(uiStrat) uiStratTreeWin& StratTreeWin();

/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

mExpClass(uiStrat) uiStratTreeWin : public uiMainWin
{ mODTextTranslationClass(uiStratTreeWin);
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

    void		popUp() const;

    static void		addTool(uiToolButtonSetup*); //!< becomes mine
    static void		makeEditable(bool allow); 
    
protected:

    uiStratRefTree*		uitree_;
    uiStratDisplay*		uistratdisp_;
    uiStratLvlList*		lvllist_;
    Strat::RepositoryAccess&	repos_;
    uiAction*			expandmnuitem_;
    uiAction*			editmnuitem_;
    uiAction*			savemnuitem_;
    uiAction*			saveasmnuitem_;
    uiAction*			resetmnuitem_;
    uiToolBar*			tb_;
    uiToolButton*		colexpbut_;
    uiToolButton*		lockbut_;
    uiToolButton*		newbut_;
    uiToolButton*		savebut_;
    uiToolButton*		moveunitupbut_;
    uiToolButton*		moveunitdownbut_;
    uiToolButton*		switchviewbut_;
    uiToolButton*		lithobut_;
    uiToolButton*		contentsbut_;
    bool			needsave_;
    bool			istreedisp_;

    void			createMenu();
    void			createToolBar();
    void			createGroups();
    void			setNewRT();
    void			setIsLocked(bool yn);
    void			setEditable(bool yn);
    bool			closeOK() override;

    void			newCB(CallBacker*);
    void			editCB(CallBacker*);
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
    void			survChgCB(CallBacker*);
    void			helpCB(CallBacker*);
    void			manLiths(CallBacker*);
    void			manConts(CallBacker*);

private:

    friend const uiStratTreeWin& StratTWin();
    static ManagedObjectSet<uiToolButtonSetup> tbsetups_;
    uiString			sExpandTxt();
    uiString			sCollapseTxt();
    uiString			sEditTxt(bool domenu);
    uiString			sLockTxt(bool domenu);

public:
    void			changeLayerModelNumber(bool add);

};

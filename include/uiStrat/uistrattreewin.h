#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uimainwin.h"
#include "manobjectset.h"
#include "menuhandler.h"
#include "multiid.h"
#include "uitoolbutton.h"

class uiAction;
class uiStratLvlList;
class uiStratRefTree;
class uiStratTreeWin;
class uiStratDisplay;
class uiToolBar;

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

    uiStratRefTree*		uitree_ = nullptr;
    uiStratDisplay*		uistratdisp_;
    uiStratLvlList*		lvllist_;
    Strat::RepositoryAccess&	repos_;

    uiToolBar*			tb_;
    uiToolBar*			stratvwtb_;
    uiToolBar*			treevwtb_;
    uiToolBar*			othertb_;

    MenuItem			newitem_;
    MenuItem			openitem_;
    MenuItem			defaultitem_;
    MenuItem			saveitem_;
    MenuItem			saveasitem_;
    MenuItem			lockitem_;
    MenuItem			resetitem_;

    MenuItem			expanditem_;
    MenuItem			collapseitem_;
    MenuItem			moveupitem_;
    MenuItem			movedownitem_;
    MenuItem			switchviewitem_;
    MenuItem			lithoitem_;
    MenuItem			contentsitem_;
    MenuItem			helpitem_;

    bool			needsave_ = false;
    bool			istreedisp_ = false;
    MultiID			treekey_;

    void			finalizeCB(CallBacker*);
    void			initWin();
    void			saveLegacyTrees();
    void			initItem(MenuItem&,const uiString&,const char*);
    void			initMenuItems();
    void			createMenu();
    void			createToolBar();
    void			createGroups();
    void			updateDisplay();
    void			updateCaption();

    void			actionCB(CallBacker*);
    void			newTree();
    void			openTree();
    void			defaultTree();
    bool			askSave();
    bool			save(bool saveas);
    void			reset();
    void			setNewRT();
    void			readTree(const MultiID&);

    void			switchView();
    void			setIsLocked(bool yn);
    bool			isLocked() const;
    void			setEditable(bool yn);
    bool			closeOK() override;

    void			updateButtonSensitivity();

    void			manLiths();
    void			manConts();
    void			help();

    void			selLvlChgCB(CallBacker*);
    void			rClickLvlCB(CallBacker*);
    void			unitSelCB(CallBacker*);
    void			unitRenamedCB(CallBacker*);
    void			survChgCB(CallBacker*);

private:

    friend const uiStratTreeWin& StratTWin();
    static ManagedObjectSet<uiToolButtonSetup> tbsetups_;
    uiString			sEditTxt(bool domenu);
    uiString			sLockTxt(bool domenu);

public:
    void			changeLayerModelNumber(bool add);

};

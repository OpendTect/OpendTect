#ifndef uistrattreewin_h
#define uistrattreewin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.45 2012-08-03 13:01:11 cvskris Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uimainwin.h"

class uiMenuItem;
class uiStratLvlList;
class uiStratRefTree;
class uiStratTreeWin;
class uiStratDisplay;
class uiToolBar;
class uiToolButton;
class uiToolButtonSetup;

namespace Strat { class RepositoryAccess; }

mGlobal(uiStrat) const uiStratTreeWin& StratTWin();
mGlobal(uiStrat) uiStratTreeWin& StratTreeWin();

/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

mClass(uiStrat) uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

    void		popUp() const;
    virtual bool	closeOK();

    static void		addTool(uiToolButtonSetup*); //!< becomes mine
    
protected:

    uiStratRefTree*		uitree_;
    uiStratDisplay*		uistratdisp_;
    uiStratLvlList*		lvllist_;
    Strat::RepositoryAccess&	repos_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			resetmnuitem_;
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
    void			forceCloseCB(CallBacker*);
    void			helpCB(CallBacker*);
    void			manLiths(CallBacker*);
    void			manConts(CallBacker*);

private:

    friend const uiStratTreeWin& StratTWin();
    static ObjectSet<uiToolButtonSetup> tbsetups_;

public:
    void			changeLayerModelNumber(bool add);

};


#endif


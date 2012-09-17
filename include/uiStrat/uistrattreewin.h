#ifndef uistrattreewin_h
#define uistrattreewin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.45 2012/07/04 10:35:30 cvsbruno Exp $
________________________________________________________________________

-*/

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
    bool			istreedisp_;

    void			initRT();
    void			createMenu();
    void			createToolBar();
    void			createGroups();

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
    void			manLiths(CallBacker*);
    void			manConts(CallBacker*);

private:

    friend const uiStratTreeWin& StratTWin();
    static ObjectSet<uiToolButtonSetup> tbsetups_;

protected:
    void                        newCB(CallBacker*);
    uiToolButton*		lithobut_;
    uiToolButton*		contentsbut_;
    uiToolButton*		newbut_;

public:
    void			doLayerModel();
    void			setIsLocked(bool yn);
    void			changeLayerModelNumber(bool add);
};


#endif

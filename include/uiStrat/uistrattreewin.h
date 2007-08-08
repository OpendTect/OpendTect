#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.4 2007-08-08 14:55:46 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiListBox;
class uiMenuItem;
class uiStratRefTree;
namespace Strat { class RefTree; }


/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

class uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

protected:

    const uiStratRefTree* 	uitree_;
    uiListBox*		 	lvllistfld_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			resetmnuitem_;

    void			createMenus();
    void			createGroups();

    void			fillLvlList();

    void			editCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			selLvlChgCB(CallBacker*);
    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);
};


#endif

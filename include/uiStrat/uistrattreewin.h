#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.6 2007-08-13 15:16:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiLabeledListBox;
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
    uiLabeledListBox*	 	lvllistfld_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			openmnuitem_;
    uiMenuItem*			resetmnuitem_;

    void			createMenus();
    void			createGroups();

    void			fillLvlList();

    void			editCB(CallBacker*);
    void			openCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			selLvlChgCB(CallBacker*);
    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);
};


#endif

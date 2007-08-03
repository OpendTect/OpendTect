#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.3 2007-08-03 15:05:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiMenuItem;
class uiStratRefTree;
class uiStratUnitDescEdTab;
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
    uiStratUnitDescEdTab*	uistrattab_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			resetmnuitem_;

    void			createMenus();

    void			editCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);
};


#endif

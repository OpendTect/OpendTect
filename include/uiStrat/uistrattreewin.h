#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.1 2007-07-09 10:12:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiMenuItem;
class uiStratRefTree;
class uiStratUnitDescTab;
namespace Strat { class RefTree; }


/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

class uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*,const Strat::RefTree*);
			~uiStratTreeWin();

protected:

    const uiStratRefTree* 	uitree_;
    uiStratUnitDescTab*		uistrattab_;
    uiMenuItem*			expandmnuitem_;

    void			createMenus();

    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);
};


#endif

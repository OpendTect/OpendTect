#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistratlvllist.h,v 1.1 2010-08-05 11:50:33 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

namespace Strat{ class Level; }
class uiStratMgr;

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*,uiStratMgr&);
				~uiStratLvlList();
protected:

    uiStratMgr&			uistratmgr_;

    void                        update(bool);
    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistratlvllist.h,v 1.2 2010-09-07 16:03:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

namespace Strat{ class Level; class UnitRepository; }

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*);
				~uiStratLvlList();
protected:

    Strat::UnitRepository& 	unitrepos_;

    void                        update(bool);
    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
 RCS:           $Id: uistratlvllist.h,v 1.4 2010-10-01 09:35:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

namespace Strat{ class Level; class LevelSet; }

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*);
				~uiStratLvlList();
protected:

    Strat::LevelSet& 		levelset_;

    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

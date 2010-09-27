#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
 RCS:           $Id: uistratlvllist.h,v 1.3 2010-09-27 11:05:19 cvsbruno Exp $
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

    void                        update(bool);
    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

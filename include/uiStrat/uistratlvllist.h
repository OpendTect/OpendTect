#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
 RCS:           $Id: uistratlvllist.h,v 1.5 2010-10-07 15:51:39 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*);
				~uiStratLvlList();
protected:

    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void			removeLvl(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

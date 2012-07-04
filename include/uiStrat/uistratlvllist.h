#ifndef uistratlvllist_h
#define uistratlvllist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
 RCS:           $Id: uistratlvllist.h,v 1.7 2012-07-04 10:36:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"

mClass uiStratLvlList : public uiLabeledListBox
{
public:

				uiStratLvlList(uiParent*);
				~uiStratLvlList();

    void			setLevels();
    void			setIsLocked(bool yn) { islocked_ = yn; }
protected:

    bool			islocked_;

    void                        editLevel(bool);

    void                        fill(CallBacker*);
    void			removeLvl(CallBacker*);
    void                        selLvlChgCB(CallBacker*);
    void                        rClickLvlCB(CallBacker*);
};


#endif

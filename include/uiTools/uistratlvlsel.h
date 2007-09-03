#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.1 2007-09-03 15:15:24 cvshelene Exp $
________________________________________________________________________

-*/

#include "callback.h"

class Color;
class BufferStringSet;
class uiGenInput;
class uiParent;
class uiPushButton;

/*!\brief used to tie a object ( Horizon, well marker... ) to a Strat::Level */

class uiStratLevelSel : public CallBacker
{
public:

			uiStratLevelSel(uiParent*);
    const char*		getLvlName() const;	
    const Color*	getLvlColor() const;

protected:
    uiGenInput*		lvlnmfld_;
    uiPushButton*	deflvlbut_;

    void		defineLvlCB(CallBacker*);
    void		fillLvlList(BufferStringSet&) const;

};

#endif

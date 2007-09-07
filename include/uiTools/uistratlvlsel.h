#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.3 2007-09-07 12:27:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class CallBacker;
class Color;
class BufferStringSet;
class uiGenInput;
class uiParent;
class uiPushButton;

/*!\brief used to tie a object ( Horizon, well marker... ) to a Strat::Level */

class uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*);
    const char*		getLvlName() const;	
    const Color*	getLvlColor() const;

    Notifier<uiStratLevelSel> selchanged_;

protected:
    uiGenInput*		lvlnmfld_;
    uiPushButton*	deflvlbut_;

    void		selLvlCB(CallBacker*);
    void		defineLvlCB(CallBacker*);
    void		fillLvlList(BufferStringSet&) const;

};

#endif

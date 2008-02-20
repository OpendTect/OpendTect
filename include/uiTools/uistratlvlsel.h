#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.4 2008-02-20 04:42:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class Color;
class uiComboBox;

/*!\brief used to tie a object ( Horizon, well marker... ) to a Strat::Level */

class uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withlabel=true,
					bool withdefine=true);
    const char*		getLvlName() const;	
    const Color*	getLvlColor() const;

    Notifier<uiStratLevelSel> levelChanged;

protected:
    uiComboBox*		lvlnmfld_;

    void		selLvlCB(CallBacker*);
    void		defineLvlCB(CallBacker*);
};

#endif

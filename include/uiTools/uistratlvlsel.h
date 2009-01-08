#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.8 2009-01-08 07:26:59 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class Color;
class uiComboBox;

namespace Strat { class Level; }

/*!\brief used to tie an object ( Horizon, Marker... ) to a Strat::Level */

mClass uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withlabel=true,
					bool withdefine=true);

    const Strat::Level*	selected() const;

    int			getID() const;
    void		setID(int);

    Color		getColor() const;
    const char*		getName() const;

    Notifier<uiStratLevelSel> selChange;

protected:

    uiComboBox*		selfld_;

    void		selCB(CallBacker*);
    void		defCB(CallBacker*);
    void		chgCB(CallBacker*);
};

#endif

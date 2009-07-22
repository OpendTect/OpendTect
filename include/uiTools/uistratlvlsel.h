#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.10 2009-07-22 16:01:23 cvsbert Exp $
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
			~uiStratLevelSel();

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

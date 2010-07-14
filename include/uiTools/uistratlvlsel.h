#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.12 2010-07-14 10:05:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class Color;
class uiComboBox;
namespace Strat { class Level; } 

/*!\brief used to tie an object ( Horizon, Marker... ) to a Unit Level */

mClass uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withlabel=true);
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
    void		chgCB(CallBacker*);
};

#endif

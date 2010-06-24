#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.11 2010-06-24 11:54:00 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class Color;
class uiComboBox;

/*!\brief used to tie an object ( Horizon, Marker... ) to a Unit Level */

mClass uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withlabel=true);
			~uiStratLevelSel();

    int			selected() const;

    int			getID() const;
    void		setID(int);

    Color		getColor() const;
    const char*		getName() const;

    Notifier<uiStratLevelSel> selChange;

protected:

    uiComboBox*		selfld_;
    TypeSet<int>	ids_;

    void		selCB(CallBacker*);
    void		defCB(CallBacker*);
    void		chgCB(CallBacker*);
};

#endif

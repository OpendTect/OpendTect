#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id: uistratlvlsel.h,v 1.6 2008-12-31 13:10:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class Color;
class uiComboBox;

namespace Strat { class Level; }

/*!\brief used to tie a object ( Horizon, well marker... ) to a Strat::Level */

class uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withlabel=true,
					bool withdefine=true);

    const Strat::Level*	selectedLevel() const;

    const Color*	getLevelColor() const;
    int			getLevelID() const;
    void		setLevelID(int);

    Notifier<uiStratLevelSel> levelChanged;

protected:
    uiComboBox*		lvlnmfld_;

    void		selLvlCB(CallBacker*);
    void		defineLvlCB(CallBacker*);
    void		lvlModif(CallBacker*);
};

#endif

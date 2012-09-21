#ifndef uistratlvlsel_h
#define uistratlvlsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class Color;
class uiComboBox;
namespace Strat { class Level; } 


/*!\brief Selector for stratigraphic levels */

mClass(uiTools) uiStratLevelSel : public uiGroup
{
public:

			uiStratLevelSel(uiParent*,bool withudf,
					const char* lbltxt=sTiedToTxt());
					//!< pass null for no label
			~uiStratLevelSel();

    const Strat::Level*	selected() const;
    const char*		getName() const;
    Color		getColor() const;
    int			getID() const;

    void		setSelected(const Strat::Level*);
    void		setName(const char*);
    void		setID(int);

    Notifier<uiStratLevelSel> selChange;

    static const char*	sTiedToTxt();

protected:

    uiComboBox*		fld_;
    bool		haveudf_;

    void		selCB(CallBacker*);
    void		extChgCB(CallBacker*);
};


#endif


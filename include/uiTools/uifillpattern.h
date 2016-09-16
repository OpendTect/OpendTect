#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2012
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "draw.h"
class uiComboBox;
class uiRectItem;



/*! \brief one-line element for fill pattern selection. No text label. */


mExpClass(uiTools) uiFillPattern : public uiGroup
{
public:

    				uiFillPattern(uiParent*);

    OD::FillPattern			get() const;
    void			set(const OD::FillPattern&);

    Notifier<uiFillPattern> 	patternChanged;

protected:

    uiComboBox*			typefld_;
    uiComboBox*			optfld_;
    uiRectItem*			patrect_;

    void			reDrawPattern();
    void			setOptNms();

    void			selChg(CallBacker*);

};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				~uiFillPattern();

    FillPattern			get() const;
    void			set(const FillPattern&);

    Notifier<uiFillPattern> 	patternChanged;

protected:

    uiComboBox*			typefld_;
    uiComboBox*			optfld_;
    uiRectItem*			patrect_;

    void			reDrawPattern();
    void			setOptNms();

    void			selChg(CallBacker*);

};

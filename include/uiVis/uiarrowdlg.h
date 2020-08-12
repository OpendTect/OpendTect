#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/


#include "uivismod.h"
#include "uidialog.h"

class uiLabeledComboBox;
class uiSelLineStyle;
class uiSlider;

mExpClass(uiVis) uiArrowDialog : public uiDialog
{mODTextTranslationClass(uiArrowDialog)
public:
			uiArrowDialog(uiParent* p);

    void		setColor(const Color&);
    const Color&	getColor() const;

    void		setLineWidth(int);
    int			getLineWidth() const;
    
    void		setArrowType(int);
    int			getArrowType() const;

    void		setScale(float);
    float		getScale() const;

    Notifier<uiArrowDialog> propertyChange;

protected:

    uiLabeledComboBox*	typefld_;
    uiSelLineStyle*	linestylefld_;
    uiSlider*		scalefld_;

    void		changeCB(CallBacker*);
};


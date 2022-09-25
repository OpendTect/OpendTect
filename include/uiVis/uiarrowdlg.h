#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			uiArrowDialog(uiParent*);
			~uiArrowDialog();

    void		setColor(const OD::Color&);
    const OD::Color&	getColor() const;

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

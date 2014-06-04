#ifndef uiarrowdlg_h
#define uiarrowdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uiarrowdlg.h 34626 2014-05-14 05:14:49Z ranojay.sen@dgbes.com $
________________________________________________________________________

-*/


#include "uivismod.h"
#include "uidialog.h"

class uiLabeledComboBox;
class uiSelLineStyle;
class uiSlider;

mExpClass(uiVis) uiArrowDialog : public uiDialog
{
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

#endif

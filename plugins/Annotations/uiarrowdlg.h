#ifndef uiarrowdlg_h
#define uiarrowdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id: uiarrowdlg.h,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class Color;
class uiLabeledComboBox;
class uiSelLineStyle;
class uiSliderExtra;

class uiArrowDialog : public uiDialog
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
    uiSliderExtra*	scalefld_;

    void		changeCB(CallBacker*);
};

#endif

#ifndef uisellinest_h
#define uisellinest_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.h,v 1.13 2009-11-13 03:17:05 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiComboBox;
class uiColorInput;
class uiLabeledSpinBox;
class LineStyle;


/*!\brief Group for defining line properties
Provides selection of linestyle, linecolor and linewidth
*/

mClass uiSelLineStyle : public uiGroup
{ 	
public:
				uiSelLineStyle(uiParent*,const LineStyle&,
					       const char* txt=0,
					       bool withdrawstyle=true,
					       bool withcolor=true,
					       bool withwidth=true);
				~uiSelLineStyle();

    void			setStyle(const LineStyle&);
    const LineStyle&		getStyle() const;

    void			setColor(const Color&);
    const Color&		getColor() const;
    void			setWidth(int);
    int				getWidth() const;
    void			setType(int);
    int				getType() const;

    Notifier<uiSelLineStyle>	changed;

protected:

    uiComboBox*			stylesel;
    uiColorInput*		colinp;
    uiLabeledSpinBox*		widthbox;

    LineStyle&			linestyle;

    void			changeCB(CallBacker*);
};

#endif

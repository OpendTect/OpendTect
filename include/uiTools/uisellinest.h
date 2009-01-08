#ifndef uisellinest_h
#define uisellinest_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.h,v 1.11 2009-01-08 07:07:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uidialog.h"

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


/*!\brief Dialog for linestyle selection
*/

mClass LineStyleDlg : public uiDialog
{
public:
			LineStyleDlg(uiParent*,const LineStyle&,
				     const char* txt=0,
				     bool withdrawstyle=true,
				     bool withcolor=true,
				     bool withwidth=false);
    const LineStyle&	getLineStyle() const;

protected:

    uiSelLineStyle*	lsfld;
};



#endif

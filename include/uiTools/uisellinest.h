#ifndef uisellinest_h
#define uisellinest_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.h,v 1.4 2002-05-23 09:29:58 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uidobjset.h"

class uiComboBox;
class uiColorInput;
class LineStyle;
class uiLabeledSpinBox;


class uiSelLineStyle : public uiGroup
{ 	
public:
			uiSelLineStyle(uiParent*,const LineStyle&,
				       const char* txt=0,bool withcolor=true,
				       bool withwidth=false);

    LineStyle		getStyle() const;

    virtual bool        isSingleLine() const { return true; }


protected:

    uiComboBox*		stylesel;
    uiColorInput*	colinp;
    uiLabeledSpinBox*	widthbox;

    const LineStyle&	ls;

};


#endif

#ifndef uisellinest_h
#define uisellinest_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.h,v 1.3 2002-05-22 11:03:59 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uidobjset.h"

class uiComboBox;
class uiColorInput;
class LineStyle;


class uiSelLineStyle : public uiGroup
{ 	
public:
			uiSelLineStyle(uiParent*,const LineStyle&,
				       const char* txt=0,bool withcolor=true);

    LineStyle		getStyle() const;

    virtual bool        isSingleLine() const { return true; }


protected:

    uiComboBox*		stylesel;
    uiColorInput*	colinp;

    const LineStyle&	ls;

};


#endif

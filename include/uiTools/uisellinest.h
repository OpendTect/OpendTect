#ifndef uisellinest_h
#define uisellinest_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.h,v 1.1 2000-11-27 10:19:43 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "draw.h"
#include "uidobjset.h"

class uiComboBox;
class uiColorInput;
class LineStyle;


class uiSelLineStyle : public uiGroup
{ 	
public:
			uiSelLineStyle(uiObject*,const LineStyle&,
				       const char* txt=0);

    LineStyle		getStyle() const;

    virtual bool        isSingleLine() const { return true; }


protected:

    uiComboBox*		stylesel;
    uiColorInput*	colinp;

    const LineStyle&	ls;

};


#endif

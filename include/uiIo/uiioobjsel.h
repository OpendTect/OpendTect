#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.2 2001-05-05 16:33:13 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
class CtxtIOObj;

/*! \brief UI element for selection of IOObjs */

class uiIOObjSelect : public uiIOSelect
{
public:
			uiIOObjSelect(uiObject*,CtxtIOObj&);
			~uiIOObjSelect();

protected:

    CtxtIOObj&		ctio;

    void		doObjSel(CallBacker*);

};


#endif

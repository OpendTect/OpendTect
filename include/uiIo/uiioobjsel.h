#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.1 2001-05-03 10:17:14 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
class CtxtIOObj;

/*! \brief UI element for selection of IOObjs

*/

//class uiIOObjSelect : public uiIOSelect
class uiIOObjSelect : public uiIOFileSelect
{
public:
			uiIOObjSelect(uiObject*);
			// uiIOObjSelect(uiObject*,CtxtIOObj&);
			~uiIOObjSelect();

protected:

};


#endif

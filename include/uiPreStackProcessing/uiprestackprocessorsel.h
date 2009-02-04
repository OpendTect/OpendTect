#ifndef uiprestackprocessorsel_h
#define uiprestackprocessorsel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Jan 2009
 RCS:		$Id: uiprestackprocessorsel.h,v 1.1 2009-02-04 16:48:00 cvskris Exp $
________________________________________________________________________


-*/

#include "uigroup.h"

class CtxtIOObj;
class uiIOObjSel;
class uiPushButton;
class MultiID;

namespace PreStack
{

mClass uiProcSel : public uiGroup
{
public:
    			uiProcSel(uiParent*,const char* label,
					  const MultiID*);
    void		setSel(const MultiID&);
    bool		getSel(MultiID&) const;

protected:
    			~uiProcSel();

    void		editPushCB(CallBacker*);
    void		selDoneCB(CallBacker*);

    CtxtIOObj&		ctio_;
    uiIOObjSel*		selfld_;
    uiPushButton*	editbut_;
};


}; //namespace

#endif

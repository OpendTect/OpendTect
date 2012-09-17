#ifndef uiprestackprocessorsel_h
#define uiprestackprocessorsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2009
 RCS:		$Id: uiprestackprocessorsel.h,v 1.3 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "uigroup.h"

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

    uiIOObjSel*		selfld_;
    uiPushButton*	editbut_;
};


}; //namespace

#endif

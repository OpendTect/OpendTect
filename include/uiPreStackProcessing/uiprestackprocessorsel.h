#ifndef uiprestackprocessorsel_h
#define uiprestackprocessorsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2009
 RCS:		$Id: uiprestackprocessorsel.h,v 1.4 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uigroup.h"

class uiIOObjSel;
class uiPushButton;
class MultiID;

namespace PreStack
{

mClass(uiPreStackProcessing) uiProcSel : public uiGroup
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


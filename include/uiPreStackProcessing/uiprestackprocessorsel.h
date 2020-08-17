#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uigroup.h"

class uiIOObjSel;
class uiPushButton;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiProcSel : public uiGroup
{ mODTextTranslationClass(uiProcSel);
public:
			uiProcSel(uiParent*,const uiString& label,
				  const MultiID*,bool withedit=true);
    void		setSel(const MultiID&);
    bool		getSel(MultiID&) const;

    Notifier<uiProcSel> selectionDone;

protected:
			~uiProcSel();

    void		editPushCB(CallBacker*);
    void		selDoneCB(CallBacker*);

    uiIOObjSel*		selfld_;
    uiPushButton*	editbut_;
};


} // namespace PreStack


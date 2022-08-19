#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

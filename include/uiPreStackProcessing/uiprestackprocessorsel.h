#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2009
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
				  const DBKey*,bool withedit=true);
    void		setSel(const DBKey&);
    bool		getSel(DBKey&) const;

    Notifier<uiProcSel> selectionDone;

protected:
			~uiProcSel();

    void		editPushCB(CallBacker*);
    void		selDoneCB(CallBacker*);

    uiIOObjSel*		selfld_;
    uiPushButton*	editbut_;
};


} // namespace PreStack

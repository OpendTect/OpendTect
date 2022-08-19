#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "multiid.h"

class uiListBox;
class uiPushButton;
class BufferStringSet;

mExpClass(uiEarthModel) uiHorizonRelationsDlg : public uiDialog
{ mODTextTranslationClass(uiHorizonRelationsDlg);
public:
			uiHorizonRelationsDlg(uiParent*,bool is2d);

protected:
    uiListBox*		relationfld_;
    uiPushButton*	crossbut_;
    uiPushButton*	waterbut_;

    BufferStringSet	hornames_;
    TypeSet<MultiID>	horids_;
    bool		is2d_;

    void		fillRelationField(const BufferStringSet&);

    void		clearCB(CallBacker*);
    void		readHorizonCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
};

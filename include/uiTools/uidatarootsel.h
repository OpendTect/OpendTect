#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiLineEdit;
class uiPushButton;

/*! \brief uiGroup for selecting the survey data root */

mExpClass(uiTools) uiDataRootSel : public uiGroup
{ mODTextTranslationClass(uiDataRootSel);
public:
				uiDataRootSel(uiParent*);
				~uiDataRootSel();

    BufferString		getDataRoot() const	{ return dataroot_; }

    Notifier<uiDataRootSel>	selectionChanged;
protected:
    uiLineEdit*			datarootlbl_;
    BufferString		dataroot_;

    void			dataRootSelCB(CallBacker*);
    void			dataRootInfoCB(CallBacker*);
};

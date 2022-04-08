#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2022
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

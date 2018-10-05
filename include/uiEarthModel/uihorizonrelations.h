#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "dbkey.h"

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
    DBKeySet		horids_;
    bool		is2d_;

    void		fillRelationField(const BufferStringSet&);

    void		clearCB(CallBacker*);
    void		readHorizonCB(CallBacker*);
    void		checkCrossingsCB(CallBacker*);
    void		makeWatertightCB(CallBacker*);
};

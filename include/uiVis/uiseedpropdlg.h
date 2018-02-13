#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "draw.h"

namespace EM { class Object; }
class uiMarkerStyle3D;


mExpClass(uiVis) uiSeedPropDlg : public uiDialog
{ mODTextTranslationClass(uiSeedPropDlg)
public:
			uiSeedPropDlg(uiParent*,EM::Object&);

protected:

    EM::Object&		emobject_;
    uiMarkerStyle3D*	stylefld_;

    OD::MarkerStyle3D	markerstyle_;

    void		styleSel(CallBacker*);
};

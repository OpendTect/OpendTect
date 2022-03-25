#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uimarkerstyledlg.h"
#include "emobject.h"

class uiColorInput;

mExpClass(uiVis) uiSeedPropDlg : public uiMarkerStyleDlg
{ mODTextTranslationClass(uiSeedPropDlg);
public:
			uiSeedPropDlg(uiParent*,EM::EMObject*,
					int posattr=EM::EMObject::sSeedNode());

protected:

    EM::EMObject*	emobject_;
    MarkerStyle3D	markerstyle_;
    int			posattr_;

    void		doFinalize(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
    void		updateMarkerStyle();
};


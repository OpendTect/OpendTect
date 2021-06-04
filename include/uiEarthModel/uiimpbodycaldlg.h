#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          April 2012
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiGenInput;
namespace EM	{ class Body; class ImplicitBody; }


/*! \brief UI for volume calculation of implicit body */

mExpClass(uiEarthModel) uiImplBodyCalDlg : public uiDialog
{ mODTextTranslationClass(uiImplBodyCalDlg);
public:
			uiImplBodyCalDlg(uiParent*,const EM::Body&);
			~uiImplBodyCalDlg();

protected:
    
    void		calcCB(CallBacker*);
    void		getImpBody();

    const EM::Body&	embody_;
    EM::ImplicitBody*	impbody_;
    uiGenInput*		velfld_;
    uiGenInput*		volfld_;
};



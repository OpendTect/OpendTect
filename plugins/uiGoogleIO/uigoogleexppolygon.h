#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id$
-*/


#include "uigoogleexpdlg.h"

class uiGenInput;
class uiSelLineStyle;
class uiIOObjSelGrp;
namespace Pick { class Set; }


mExpClass(uiGoogleIO) uiGISExportPolygon : public uiDialog
{ mODTextTranslationClass(uiGISExportPolygon);
public:
			uiGISExportPolygon(uiParent*,
					    const MultiID& mid=MultiID::udf());
			~uiGISExportPolygon();

protected:

    uiGISExpStdFld*	expfld_;
    uiIOObjSelGrp*	selfld_			= nullptr;
    uiGenInput*		inputyp_		= nullptr;

    bool		acceptOK(CallBacker*);
    void		inputTypChngCB(CallBacker*);
    MultiID		mid_;
};

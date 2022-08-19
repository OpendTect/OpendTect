#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigroup.h"

class uiFileInput;
class uiGenInput;
namespace Coords { class uiCoordSystemSel; }
namespace GIS { class Writer; }


mExpClass(uiTools) uiGISExpStdFld : public uiGroup
{
mODTextTranslationClass(uiGISExpStdFld)
public:
				uiGISExpStdFld(uiParent*,const char* typnm);
				~uiGISExpStdFld();

    PtrMan<GIS::Writer>		createWriter(const char* survnm,
					     const char* elemnm) const;

    static bool			canDoExport(uiParent*,SurveyInfo* =nullptr);
    static uiString		sToolTipTxt();
    static const char*		strIcon();
    static OD::Color		sDefColor();
    static int			sDefLineWidth();

private:

    void			initGrpCB(CallBacker*);
    void			typeChgCB(CallBacker*);

    uiFileInput*		fnmfld_;
    uiGenInput*			exptyp_;
    Coords::uiCoordSystemSel*	coordsysselfld_ = nullptr;

};

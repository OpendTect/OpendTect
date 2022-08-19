#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleiomod.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "giswriter.h"
#include "geojsonwriter.h"
#include "uigroup.h"
#include "uicoordsystem.h"

class SurveyInfo;
class uiSurveyManager;

mClass(uiGoogleIO) uiGISExpStdFld : public uiGroup
{ mODTextTranslationClass(uiGISExpStdFld)
public:
			    uiGISExpStdFld(uiParent*,BufferString typnm);
			    ~uiGISExpStdFld();

    GISWriter*			createWriter() const;

protected:

    void			expTypChng(CallBacker*);

    uiFileInput*		fnmfld_;
    uiGenInput*			exptyp_;
    Coords::uiCoordSystemSel*	coordsysselfld_;


};

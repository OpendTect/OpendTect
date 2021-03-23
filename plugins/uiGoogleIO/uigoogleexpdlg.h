#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id$
-*/

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

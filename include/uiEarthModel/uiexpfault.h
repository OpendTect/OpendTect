#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2008
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "dbkey.h"
class CtxtIOObj;
class uiCheckBox;
class uiCheckList;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiUnitSel;
class uiIOObjSelGrp;
class StreamData;

/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportFault : public uiDialog
{ mODTextTranslationClass(uiExportFault);
public:
			uiExportFault(uiParent*,const char* type,
							bool issingle=true);
			~uiExportFault();

protected:

    uiIOObjSelGrp*	bulkinfld_;
    uiIOObjSel*		singleinfld_;
    uiGenInput*		coordfld_;
    uiUnitSel*		zunitsel_;
    uiCheckList*	stickidsfld_;
    uiCheckBox*		linenmfld_;
    uiFileInput*	outfld_;

    CtxtIOObj&		ctio_;
    bool		getInputDBKeys(DBKeySet&);
    virtual bool	acceptOK();
    bool		writeAscii();
    bool		issingle_;
};

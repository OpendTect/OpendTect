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
class uiFileSel;
class uiGenInput;
class uiIOObjSel;
class uiUnitSel;
class uiIOObjSelGrp;
class StreamData;
class uiT2DConvSel;

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
    uiFileSel*		outfld_;
    uiGenInput*		zfld_;
    uiT2DConvSel*	transfld_;

    CtxtIOObj&		ctio_;
    bool		getInputDBKeys(DBKeySet&);

    void		addZChg(CallBacker*);
    FixedString		getZDomain() const;

    virtual bool	acceptOK();
    bool		writeAscii();
    bool		issingle_;
};

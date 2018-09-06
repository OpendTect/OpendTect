#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "fixedstring.h"
#include "uiioobjselgrp.h"
#include "zaxistransform.h"
#include "uicoordsystem.h"

class uiFileSel;
class uiGenInput;
class uiSurfaceRead;
class uiUnitSel;
class uiButton;
class uiT2DConvSel;
class write3DHorASCII;

/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportHorizon : public uiDialog
{ mODTextTranslationClass(uiExportHorizon);
public:
			uiExportHorizon(uiParent*,bool issingle=true);
			~uiExportHorizon();



protected:

    uiSurfaceRead*		infld_;
    uiFileSel*			outfld_;
    uiGenInput*			headerfld_;
    uiGenInput*			typfld_;
    uiGenInput*			zfld_;
    uiButton*			settingsbutt_;
    uiUnitSel*			unitsel_;
    uiGenInput*			udffld_;
    uiT2DConvSel*		transfld_;
    uiIOObjSelGrp*		bulkinfld_;
    Coords::uiCoordSystemSel*	coordsysselfld_;


    BufferString		gfname_;
    BufferString		gfcomment_;

    virtual bool		acceptOK();
    void			typChg(CallBacker*);
    void			addZChg(CallBacker*);
    void			attrSel(CallBacker*);
    void			settingsCB(CallBacker*);
    void			inpSel(CallBacker*);
    void			writeHeader(od_ostream&);
    bool			writeAscii();
    bool			getInputDBKeys(DBKeySet&);

    bool			isbulk_;

    FixedString			getZDomain() const;
};

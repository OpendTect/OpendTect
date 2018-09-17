#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "fixedstring.h"
#include "uiioobjselgrp.h"
#include "uicoordsystem.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;
class uiUnitSel;
class uiPushButton;
class uiT2DConvSel;

/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportHorizon : public uiDialog
{ mODTextTranslationClass(uiExportHorizon);
public:
			uiExportHorizon(uiParent*,bool isbulk=false);
			~uiExportHorizon();


protected:

    uiSurfaceRead*	infld_;
    uiFileInput*	outfld_;
    uiGenInput*		headerfld_;
    uiGenInput*		typfld_;
    uiGenInput*		zfld_;
    uiPushButton*	settingsbutt_;
    uiUnitSel*		unitsel_;
    uiGenInput*		udffld_;
    uiT2DConvSel*	transfld_;
    uiIOObjSelGrp*	bulkinfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    BufferString	gfname_;
    BufferString	gfcomment_;

    virtual bool	acceptOK(CallBacker*);
    void		typChg(CallBacker*);
    void		addZChg(CallBacker*);
    void		attrSel(CallBacker*);
    void		settingsCB(CallBacker*);
    void		inpSel(CallBacker*);
    void		writeHeader(od_ostream&);
    bool		writeAscii();
    bool		getInputMIDs(TypeSet<MultiID>&);

    bool		isbulk_;

    FixedString		getZDomain() const;
};


#endif

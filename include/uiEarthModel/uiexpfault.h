#ifndef uiexpfault_h
#define uiexpfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "multiid.h"
#include "uicoordsystem.h"
#include "emobject.h"

class CtxtIOObj;
class uiCheckBox;
class uiCheckList;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class StreamData;
class uiUnitSel;
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
    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiCheckList*	stickidsfld_;
    uiCheckBox*		linenmfld_;
    uiFileInput*	outfld_;
    uiUnitSel*		zunitsel_;
    uiGenInput*		zfld_;
    uiT2DConvSel*	transfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    CtxtIOObj&		ctio_;
    bool		getInputMIDs(TypeSet<MultiID>&);

    void		addZChg(CallBacker*);
    void		exportCoordSysChgCB(CallBacker*);
    FixedString		getZDomain() const;

    virtual bool	acceptOK(CallBacker*);

    bool		writeAscii();
    bool		issingle_;
    uiString		dispstr_;
};

#endif

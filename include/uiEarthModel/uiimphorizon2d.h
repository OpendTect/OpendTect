#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "emobject.h"
#include "emposid.h"
#include "multiid.h"

class BufferStringSet;
class Horizon2DScanner;
class SurfaceInfo;

class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiListBox;
class uiPushButton;
class uiIOObjSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace ZDomain { class Info; }

/*! \brief Dialog for Horizon Import */

mExpClass(uiEarthModel) uiImportHorizon2D : public uiDialog
{ mODTextTranslationClass(uiImportHorizon2D);
public:
			uiImportHorizon2D(uiParent*);
			~uiImportHorizon2D();

    Notifier<uiImportHorizon2D>	readyForDisplay;

    EM::ObjectID	getEMObjID() const;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*       scanbut_;
    uiTableImpDataSel*  dataselfld_;
    uiGenInput*		udftreatfld_;
    uiGenInput*		zdomselfld_;
    uiIOObjSel*		timeoutputfld_;
    uiIOObjSel*		depthoutputfld_;

    bool		acceptOK(CallBacker*) override;
    void                descChg(CallBacker*);
    void		setSel(CallBacker*);
    void		formatSel(CallBacker*);
    void		scanPush(CallBacker*);
    void		zDomainCB(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doImport();
    bool		isASCIIFileInTime() const;

    uiIOObjSel*		getWorkingOutFld() const;

    const ZDomain::Info& zDomain() const;

    Table::FormatDesc&	    fd_;
    Horizon2DScanner*	    scanner_;
    BufferStringSet&	    linesetnms_;
    TypeSet<MultiID>	    setids_;
    RefMan<EM::EMObject>    emobj_;
};

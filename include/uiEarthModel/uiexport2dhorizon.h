#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiioobjselgrp.h"

class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiMultiSurfaceRead;
class uiListBox;
class uiSurfaceRead;
class uiUnitSel;

class od_ostream;
class SurfaceInfo;
namespace Coords { class uiCoordSystemSel; }
namespace ZDomain { class Info; }


/*! \brief Dialog for 2D horizon export */

mExpClass(uiEarthModel) uiExport2DHorizon : public uiDialog
{ mODTextTranslationClass(uiExport2DHorizon);
public:
			uiExport2DHorizon(uiParent*,bool isbulk);
			~uiExport2DHorizon();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiMultiSurfaceRead*		    multisurfdepthread_     = nullptr;
    uiMultiSurfaceRead*		    multisurftimeread_	    = nullptr;
    uiListBox*			    linenmfld_		    = nullptr;
    uiGenInput*			    horzdomypefld_	    = nullptr;
    uiSurfaceRead*		    surfread_		    = nullptr;
    Coords::uiCoordSystemSel*	    coordsysselfld_	    = nullptr;
    uiCheckBox*			    writeudffld_;
    uiGenInput*			    udffld_;
    uiCheckBox*			    writelinenmfld_;
    uiGenInput*			    headerfld_;
    uiUnitSel*			    unitsel_;
    uiFileInput*		    outfld_;


    bool			    acceptOK(CallBacker*) override;
    void			    horChg(CallBacker*);
    void			    undefCB(CallBacker*);
    void			    zDomainTypeChg(CallBacker*);

    bool			    doExport();
    void			    writeHeader(od_ostream&);
    bool			    getInputMultiIDs(TypeSet<MultiID>&);
    bool			    isTime() const;
    const ZDomain::Info&	    zDomain() const;
    bool			    isbulk_;

};

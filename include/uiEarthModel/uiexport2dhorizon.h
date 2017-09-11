#ifndef uiexport2dhorizon_h
#define uiexport2dhorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiioobjselgrp.h"
class SurfaceInfo;
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckList;
class uiFileInput;
class od_ostream;


/*! \brief Dialog for 2D horizon export */

mExpClass(uiEarthModel) uiExport2DHorizon : public uiDialog
{ mODTextTranslationClass(uiExport2DHorizon);
public:
			uiExport2DHorizon(uiParent*,
					  const ObjectSet<SurfaceInfo>&,
					  bool isbulk=true);
			~uiExport2DHorizon();


protected:

    uiComboBox*		horselfld_;
    uiListBox*		linenmfld_;
    uiGenInput*		udffld_;
    uiFileInput*	outfld_;
    uiCheckList*	optsfld_;
    uiGenInput*		headerfld_;

    const ObjectSet<SurfaceInfo>&	hinfos_;

    virtual bool	acceptOK(CallBacker*);
    void		horChg(CallBacker*);
    bool		doExport();
    void		writeHeader(od_ostream&);
protected:

    uiIOObjSelGrp*	bulkinfld_;

    bool		isbulk_;
    bool		getInputMultiIDs(TypeSet<MultiID>&);
};


#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
class SurfaceInfo;
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckList;
class uiFileSel;
class od_ostream;


/*! \brief Dialog for 2D horizon export */

mExpClass(uiEarthModel) uiExport2DHorizon : public uiDialog
{ mODTextTranslationClass(uiExport2DHorizon);
public:
			uiExport2DHorizon(uiParent*,
					  const ObjectSet<SurfaceInfo>&);
			~uiExport2DHorizon();


protected:

    uiComboBox*		horselfld_;
    uiListBox*		linenmfld_;
    uiGenInput*		udffld_;
    uiFileSel*		outfld_;
    uiCheckList*	optsfld_;
    uiGenInput*		headerfld_;

    const ObjectSet<SurfaceInfo>&	hinfos_;

    virtual bool	acceptOK();
    void		horChg(CallBacker*);
    bool		doExport();
    void		writeHeader(od_ostream&);
};

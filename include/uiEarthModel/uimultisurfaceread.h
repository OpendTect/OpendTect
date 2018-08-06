#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiiosurface.h"
#include "uidialog.h"

class uiIOObjSelGrp;
class uiDialog;

/*! \brief ui for multiple surface read */

mExpClass(uiEarthModel) uiMultiSurfaceRead : public uiIOSurface
{ mODTextTranslationClass(uiMultiSurfaceRead);
public:
			uiMultiSurfaceRead(uiParent*,const char* type);
			~uiMultiSurfaceRead();

    uiIOObjSelGrp*	objselGrp()		{ return ioobjselgrp_;}

    void		setSurfaceIds(const DBKeySet&);
    void		getSurfaceIds(DBKeySet&) const;
    void		getSurfaceSelection(EM::SurfaceIODataSelection&) const;

    Notifier<uiMultiSurfaceRead>	singleSurfaceSelected;

protected:

    uiIOObjSelGrp*	ioobjselgrp_;

    void                dClck(CallBacker*);
    void		selCB(CallBacker*);
};


mExpClass(uiEarthModel) uiMultiSurfaceReadDlg : public uiDialog
{ mODTextTranslationClass(uiMultiSurfaceReadDlg)
public:
			uiMultiSurfaceReadDlg(uiParent*,const char* type);

    uiMultiSurfaceRead*	iogrp()		{ return surfacefld_; }

			// returned sets are reffed and must be unreffed
    static void		selectHorizons(uiParent*,ObjectSet<EM::Object>&,
					bool is2d=false);
    static void		selectFaults(uiParent*,ObjectSet<EM::Object>&);
    static void		selectFaultStickSets(uiParent*,ObjectSet<EM::Object>&);

protected:

    void		statusMsg(CallBacker*);
    bool		acceptOK();

    uiMultiSurfaceRead*	surfacefld_;

    static void		selectSurfaces(uiParent*,ObjectSet<EM::Object>&,
				       const char* typ);
};

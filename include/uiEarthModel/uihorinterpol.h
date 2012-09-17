#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id: uihorinterpol.h,v 1.7 2009/12/16 06:03:19 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "uidialog.h"

class uiGenInput;
namespace EM { class Horizon; }

class uiHorSaveFieldGrp;
class uiArray1DInterpolSel;
class uiArray2DInterpolSel;
class uiIOObjSel;


mClass uiHorizonInterpolDlg : public uiDialog
{
public:
    			uiHorizonInterpolDlg(uiParent*,EM::Horizon*,
					     bool is2d=false);
			~uiHorizonInterpolDlg();

    const char*		helpID() const;
    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }

protected:

    bool			acceptOK(CallBacker*);

    bool			interpolate3D();
    bool			interpolate2D();
    bool			is2d_;
    uiIOObjSel*			inputhorsel_;
    uiArray2DInterpolSel*	interpol2dsel_;
    uiArray1DInterpolSel*	interpol1dsel_;
    uiGenInput*			geometrysel_;
    uiHorSaveFieldGrp*          savefldgrp_;

    EM::Horizon*		horizon_;
};


#endif

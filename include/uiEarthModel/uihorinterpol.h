#ifndef uihorinterpol_h
#define uihorinterpol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2009
 RCS:		$Id: uihorinterpol.h,v 1.6 2009-11-04 16:01:05 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "uidialog.h"

class uiGenInput;
namespace EM { class Horizon3D; }

class uiHorSaveFieldGrp;
class Array2DInterpol;
class uiArray2DInterpolSel;
class uiIOObjSel;


mClass uiHorizon3DInterpolDlg : public uiDialog
{
public:
    			uiHorizon3DInterpolDlg(uiParent*,EM::Horizon3D*);
			~uiHorizon3DInterpolDlg();

    const char*		helpID() const;
    uiHorSaveFieldGrp*	saveFldGrp() const { return savefldgrp_; }

protected:

    bool			acceptOK(CallBacker*);

    uiIOObjSel*			inputhorsel_;
    uiArray2DInterpolSel*	interpolsel_;
    uiGenInput*			geometrysel_;
    uiHorSaveFieldGrp*          savefldgrp_;

    EM::Horizon3D*		horizon_;
};


#endif

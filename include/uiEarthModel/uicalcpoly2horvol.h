#ifndef uicalcpoly2horvol_h
#define uicalcpoly2horvol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
 RCS:           $Id: uicalcpoly2horvol.h,v 1.2 2009-08-12 06:14:16 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiCheckBox;
namespace Pick	{ class Set; }
namespace EM	{ class Horizon3D; }


/*! \brief Calculate isopach as attribute of horizon */

mClass uiCalcPoly2HorVol : public uiDialog
{
public:

			uiCalcPoly2HorVol(uiParent*,const Pick::Set&);
			~uiCalcPoly2HorVol();

    const Pick::Set&	pickSet()		{ return ps_; }

protected:

    uiIOObjSel*		horsel_;
    uiCheckBox*		upwbox_;
    uiCheckBox*		ignnegbox_;
    uiGenInput*		velfld_;
    uiGenInput*		valfld_;

    const Pick::Set&	ps_;
    CtxtIOObj&		ctio_;
    const bool		zinft_;
    EM::Horizon3D*	curhor_;
    
    void		horSel(CallBacker*);
    void		haveChg(CallBacker*);
    void		doCalc(CallBacker*);

    float		getM3(float);
    void		dispVal(float);

};


#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/


#include "uiearthmodelmod.h"
#include "uiselsimple.h"

class uiGenInput;
class uiHorSaveFieldGrp;
class DataPointSet;
namespace EM { class Horizon3D; }


/*!\brief Save the geometry to an attribute */

mExpClass(uiEarthModel) uiHorGeom2Attr : public uiGetObjectName
{ mODTextTranslationClass(uiHorGeom2Attr)
public:
			uiHorGeom2Attr(uiParent*,EM::Horizon3D&);
			~uiHorGeom2Attr();

protected:

    EM::Horizon3D&	hor_;
    BufferStringSet*	itmnms_;

    uiGenInput*		msfld_;

    BufferStringSet&	getItems(const EM::Horizon3D&);

    virtual bool	acceptOK(CallBacker*);

};


/*!\brief Change the geometry using an attribute */

mExpClass(uiEarthModel) uiHorAttr2Geom : public uiDialog
{ mODTextTranslationClass(uiHorAttr2Geom)
public:
			uiHorAttr2Geom(uiParent*,EM::Horizon3D&,
					const DataPointSet&,int colid);
			~uiHorAttr2Geom();

    const uiHorSaveFieldGrp*	saveFldGrp() const	{ return savefldgrp_; }

protected:

    EM::Horizon3D&	hor_;
    const DataPointSet&	dps_;
    int			colid_;

    uiGenInput*		isdeltafld_;
    uiGenInput*		msfld_;
    uiHorSaveFieldGrp*	savefldgrp_;

    virtual bool	acceptOK(CallBacker*);


};



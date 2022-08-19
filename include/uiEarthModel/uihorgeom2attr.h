#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool		acceptOK(CallBacker*) override;

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

    bool		acceptOK(CallBacker*) override;


};

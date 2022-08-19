#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"

#include "emposid.h"
#include "multiid.h"

class IOObj;
namespace Pos { class EMSurfaceProvider3D; }
class uiGenInput;
class uiSeisSel;


/*! \brief Create flattened cube from horizon */

mExpClass(uiEMAttrib) uiWriteFlattenedCube : public uiDialog
{
mODTextTranslationClass(uiWriteFlattenedCube)
public:

			uiWriteFlattenedCube(uiParent*,EM::ObjectID);
			~uiWriteFlattenedCube();

protected:

    uiSeisSel*		seisselin_;
    uiSeisSel*		seisselout_;
    uiGenInput*		zvalfld_;

    MultiID		hormid_;
    float		defzval_;
    Interval<float>	horzrg_;
    Pos::EMSurfaceProvider3D& pp_;

    bool		acceptOK(CallBacker*) override;
    BufferString	getHorNm(EM::ObjectID);

    bool		doWork(const IOObj&,const IOObj&,float);

    friend class	uiWriteFlattenedCubeWriter;
};

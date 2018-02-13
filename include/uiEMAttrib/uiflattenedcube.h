#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "dbkey.h"

class IOObj;
namespace Pos { class EMSurfaceProvider3D; }
class uiGenInput;
class uiSeisSel;


/*! \brief Create flattened cube from horizon */

mClass(uiEMAttrib) uiWriteFlattenedCube : public uiDialog
{ mODTextTranslationClass(uiWriteFlattenedCube);
public:

			uiWriteFlattenedCube(uiParent*,const DBKey&);
			~uiWriteFlattenedCube();

protected:

    uiSeisSel*		seisselin_;
    uiSeisSel*		seisselout_;
    uiGenInput*		zvalfld_;

    DBKey		hormid_;
    float		defzval_;
    Interval<float>	horzrg_;
    Pos::EMSurfaceProvider3D& pp_;
    uiString		errmsg_;

    bool		acceptOK();
    BufferString	getHorNm(const DBKey&);

    bool		doWork(const IOObj&,const IOObj&,float);

    friend class	uiWriteFlattenedCubeWriter;
};

#ifndef uiflattenedcube_h
#define uiflattenedcube_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
 RCS:           $Id: uiflattenedcube.h,v 1.4 2009/07/22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"
#include "multiid.h"
class uiSeisSel;
class uiGenInput;
class CtxtIOObj;
namespace EM { class Horizon3D; }
namespace Pos { class EMSurfaceProvider3D; }


/*! \brief Create flattened cube from horizon */

class uiWriteFlattenedCube : public uiDialog
{
public:

			uiWriteFlattenedCube(uiParent*,EM::ObjectID);
			~uiWriteFlattenedCube();

protected:

    uiSeisSel*		seisselin_;
    uiSeisSel*		seisselout_;
    uiGenInput*		zvalfld_;

    MultiID		hormid_;
    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
    float		defzval_;
    Interval<float>	horzrg_;
    Pos::EMSurfaceProvider3D& pp_;

    bool		acceptOK(CallBacker*);
    BufferString	getHorNm(EM::ObjectID);

    bool		doWork(float);

    friend class	uiWriteFlattenedCubeWriter;
};


#endif

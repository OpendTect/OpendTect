#ifndef uiflattenedcube_h
#define uiflattenedcube_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          May 2008
 RCS:           $Id: uiflattenedcube.h,v 1.1 2008-05-21 09:24:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"
#include "multiid.h"
class uiSeisSel;
class uiGenInput;
class CtxtIOObj;
namespace EM { class Horizon3D; }


/*! \brief UI for Quick UVQ on horizon */

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

    bool		acceptOK(CallBacker*);
    BufferString	getHorNm(EM::ObjectID);

    bool		doWork(float);

    friend class	uiWriteFlattenedCubeWriter;
};


#endif

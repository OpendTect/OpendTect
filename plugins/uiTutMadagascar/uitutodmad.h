#ifndef uitutodmad_h
#define uitutodmad_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          June 2009
 RCS:           $Id: uitutodmad.h,v 1.2 2009-06-06 12:58:09 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "iopar.h"

class SeisTrcBufDataPack;
class uiFileInput;
class uiGenInput;

/*! \brief Madagascar tutorial plugin interface */

class uiTutODMad : public uiDialog
{
public:

			uiTutODMad(uiParent*);

protected:

    void		createAndDisplay2DViewer();
    bool		acceptOK(CallBacker*);

    uiFileInput*	maddatafld_;
    uiGenInput*		dowigglesfld_;
    IOPar		iop_;
    SeisTrcBufDataPack* bufdtpack_;
};


#endif

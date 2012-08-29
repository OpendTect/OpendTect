#ifndef uitutodmad_h
#define uitutodmad_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          June 2009
 RCS:           $Id: uitutodmad.h,v 1.4 2012-08-29 08:18:07 cvskris Exp $
________________________________________________________________________

-*/

#include "uitutmadagascarmod.h"
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

#ifndef uivelocityvolumeconversion_h
#define uivelocityvolumeconversion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uivelocityvolumeconversion.h,v 1.1 2009-12-08 16:27:26 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"


class CtxtIOObj;
class uiVelSel;
class uiSeisSel;
class uiPosSubSel;

namespace Vel
{

/*!Dialog to setup a velocity conversion for volumes on disk. */

class uiBatchVolumeConversion : public uiFullBatchDialog
{
public:
    		uiBatchVolumeConversion(uiParent*);

protected:

    void		inputChangeCB(CallBacker*);
    bool		prepareProcessing() { return true; }
    bool		fillPar(IOPar&);

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiSeisSel*		outputsel_;
};

}; //namespace



#endif

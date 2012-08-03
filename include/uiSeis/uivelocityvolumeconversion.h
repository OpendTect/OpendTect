#ifndef uivelocityvolumeconversion_h
#define uivelocityvolumeconversion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uivelocityvolumeconversion.h,v 1.4 2012-08-03 13:01:10 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uibatchlaunch.h"


class CtxtIOObj;
class uiVelSel;
class uiSeisSel;
class uiPosSubSel;
class uiLabeledComboBox;

namespace Vel
{

/*!Dialog to setup a velocity conversion for volumes on disk. */

mClass(uiSeis) uiBatchVolumeConversion : public uiFullBatchDialog
{
public:
			uiBatchVolumeConversion(uiParent*);

protected:

    void		inputChangeCB(CallBacker*);
    bool		prepareProcessing() { return true; }
    bool		fillPar(IOPar&);

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiLabeledComboBox*	outputveltype_;
    uiSeisSel*		outputsel_;
};

}; //namespace



#endif


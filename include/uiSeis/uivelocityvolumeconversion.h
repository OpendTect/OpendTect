#ifndef uivelocityvolumeconversion_h
#define uivelocityvolumeconversion_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class uiBatchJobDispatcherSel;
class uiVelSel;
class uiSeisSel;
class uiPosSubSel;
class uiLabeledComboBox;

/*!\brief Velocity*/

namespace Vel
{

/*!Dialog to setup a velocity conversion for volumes on disk. */

mExpClass(uiSeis) uiBatchVolumeConversion : public uiDialog
{
public:
			uiBatchVolumeConversion(uiParent*);

protected:

    void		inputChangeCB(CallBacker*);
    bool		fillPar();
    bool		acceptOK(CallBacker*);

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiLabeledComboBox*	outputveltype_;
    uiSeisSel*		outputsel_;
    uiBatchJobDispatcherSel*	batchfld_;
};

}; //namespace



#endif


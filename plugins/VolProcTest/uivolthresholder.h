#ifndef uivolthresholder_h
#define uivolthresholder_h

/*+

_________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id$
_________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;

namespace VolProc
{

class ProcessingStep;
class ThresholdStep;

class uiVolumeThresholder : public uiDialog
{
public:
    static void		initClass();
    			
			uiVolumeThresholder(uiParent*,ThresholdStep*);

protected:
    static uiDialog*	create(uiParent*, ProcessingStep*);
    bool		acceptOK(CallBacker*);

    ThresholdStep*	thresholdstep_;
    uiGenInput*		thresholdfld_; 
};


}; //namespace
#endif

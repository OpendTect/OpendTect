#ifndef uivolthresholder_h
#define uivolthresholder_h

/*+

_________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id: uivolthresholder.h,v 1.2 2009/07/22 16:01:27 cvsbert Exp $
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

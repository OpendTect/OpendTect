#ifndef uivolthresholder_h
#define uivolthresholder_h

/*+

_________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id: uivolthresholder.h,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $
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

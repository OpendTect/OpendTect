#ifndef uivolprocfaultangle_h
#define uivolprocfaultangle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: uivolprocfaultangle.h,v 1.1 2012-08-20 21:05:52 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprocfaultangle.h"

class uiIOObjSel;
class uiLabeledSpinBox;


namespace VolProc
{

mClass(uiVolumeProcessing) uiFaultAngle : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::FaultAngle::sFactoryKeyword(),
	    VolProc::FaultAngle::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );
protected:


				uiFaultAngle(uiParent*,VolProc::FaultAngle*);
   				~uiFaultAngle();
    static uiStepDialog*	createInstance(uiParent*,Step*);

    bool			acceptOK(CallBacker*);
    void			mergeChgCB(CallBacker*);

    VolProc::FaultAngle*	fltaz_;
    uiGenInput*			isazimuthfld_;
    uiGenInput*			thresholdfld_;
    uiGenInput*			isabovefld_;
    uiGenInput*			overlapratefld_;
    uiGenInput*			domergefld_;
    uiGenInput*			dothinningfld_;
    uiLabeledSpinBox*		minlengthfld_;
};

}; //namespace

#endif


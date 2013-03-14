#ifndef uilinearveltrans_h
#define uilinearveltrans_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2013
 RCS:           $Id: uiselectvelocityfunction.h 28682 2013-03-02 21:12:38Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uizaxistransform.h"
#include "uivelocitymod.h"

class uiGenInput;
class uiZRangeInput;

namespace Vel
{

mExpClass(uiVelocity) uiLinearVelTransform : public uiZAxisTransform
{
public:
    static void		initClass();
    			uiLinearVelTransform(uiParent*,bool t2d);

    void                enableTargetSampling();
    bool                getTargetSampling(StepInterval<float>&) const;

    ZAxisTransform*	getSelection();

protected:
    static uiZAxisTransform*	create(uiParent*,const char*,const char*);
    bool			acceptOK();
    void			velChangedCB(CallBacker*);
    void			rangeChangedCB(CallBacker*);
    void			finalizeDoneCB(CallBacker*);

    uiGenInput*			velfld_;
    uiGenInput*			gradientfld_;
    uiZRangeInput*		rangefld_;

    bool			t2d_;
    bool			rangechanged_;
};

}; //namespace


#endif


#ifndef uizaxistransform_h
#define uizaxistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "factory.h"
#include "fixedstring.h"
#include "uidlggroup.h"

class ZAxisTransform;
class uiGenInput;
class uiPushButton;
class uiDialog;

/*! Base class for ZAxisTransform ui's*/

mExpClass(uiTools) uiZAxisTransform : public uiDlgGroup
{
public:
    mDefineFactory3ParamInClass(uiZAxisTransform,uiParent*,
	    			const char*,const char*,factory);
    
    virtual void		enableTargetSampling();
    virtual bool		getTargetSampling(StepInterval<float>&) const;

    virtual ZAxisTransform*	getSelection()			= 0;

protected:
    				uiZAxisTransform(uiParent*);
};


/*!Selects a ZAxisTransform. */
mExpClass(uiTools) uiZAxisTransformSel : public uiDlgGroup
{
public:
    				uiZAxisTransformSel(uiParent*, bool withnone,
						    const char* fromdomain=0,
						    const char* todomain=0,
						    bool withsampling=false);
    
    bool			isOK() const;
    
    bool			fillPar(IOPar&);
    ZAxisTransform*		getSelection();
    NotifierAccess*		selectionDone();
    int				nrTransforms() const;

    bool			acceptOK();
    
    bool			getTargetSampling(StepInterval<float>&) const;

protected:
    void			selCB(CallBacker*);

    BufferString		fromdomain_;
    uiGenInput*			selfld_;
    ObjectSet<uiZAxisTransform>	transflds_;
};


#endif


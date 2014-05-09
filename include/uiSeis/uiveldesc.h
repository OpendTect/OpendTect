#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uizaxistransform.h"
#include "veldesc.h"
#include "timedepthconv.h"

class uiSeisSel;
class uiStaticsDesc;
class uiZRangeInput;
class VelocityStretcher;

/*!Group that allows the user to edit VelocityDesc information. */

mExpClass(uiSeis) uiVelocityDesc : public uiGroup
{ mODTextTranslationClass(uiVelocityDesc);
public:

    mExpClass(uiSeis) Setup
    {
    public:
				Setup( const VelocityDesc* vd=0 )
				{ if ( vd ) desc_ = *vd; }

	mDefSetupMemb(VelocityDesc,desc)
    };

    				uiVelocityDesc(uiParent*,const Setup* s=0);

    bool			get(VelocityDesc&,bool displayerrors) const;
    void			set(const VelocityDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

    NotifierAccess&		typeChangeNotifier();

protected:

    void			updateFlds(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			hasstaticsfld_;
    uiStaticsDesc*		staticsfld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */
mExpClass(uiSeis) uiVelocityDescDlg : public uiDialog
{ mODTextTranslationClass(uiVelocityDescDlg)
public:
    			uiVelocityDescDlg(uiParent*,const IOObj* cursel=0,
					  const uiVelocityDesc::Setup* s=0);
			~uiVelocityDescDlg();

    IOObj*		getSelection() const;
   			//!<returned object must be managed by caller
    Interval<float>	getVelocityTopRange() const	
    			{ return toprange_; }
    Interval<float>	getVelocityBottomRange() const	
    			{ return bottomrange_; }

protected:

   bool			acceptOK(CallBacker*);
   void			volSelChange(CallBacker*);
   bool			scanAvgVel(const IOObj&, const VelocityDesc&);

   Interval<float>	toprange_;
   Interval<float>	bottomrange_;

   VelocityDesc		oldveldesc_;
   uiSeisSel*		volselfld_;
   uiVelocityDesc*	veldescfld_;
};


//!Field that selects a velocity volume, and edit it's properties/velocity tag


mExpClass(uiSeis) uiVelSel : public uiSeisSel
{
public:
    				uiVelSel(uiParent*,const IOObjContext&,
					 const uiSeisSel::Setup&,
					 bool iseditbutton=true);

    void			setInput(const MultiID&);
    static const IOObjContext&	ioContext();
    
    Interval<float>		getVelocityTopRange() const	{ return trg_; }
    Interval<float>		getVelocityBottomRange() const	{ return brg_; }
    Notifier<uiVelSel>		velrgchanged;

protected:

    void			fillDefault();

    void			selectionDoneCB(CallBacker*);
    void			updateEditButton();
    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
    Interval<float>		trg_;
    Interval<float>		brg_;
};


mExpClass(uiSeis) uiTime2DepthZTransformBase : public uiZAxisTransform
{
public:
    FixedString 	toDomain() const;
    FixedString 	fromDomain() const;

    void		enableTargetSampling();
    bool		getTargetSampling(StepInterval<float>&) const;

protected:
			uiTime2DepthZTransformBase(uiParent*,bool t2d);

    bool		t2d_;
    uiZRangeInput*	rangefld_;
};


mExpClass(uiSeis) uiVelModelZAxisTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiVelModelZAxisTransform);
public:
    void			enableTargetSampling();
    bool			acceptOK();

    ZAxisTransform*		getSelection();

    const char*			selName() const;
    const MultiID&		selID() const { return selkey_; }

    bool			canBeField() const { return true; }
protected:
				uiVelModelZAxisTransform(uiParent*,bool);
				~uiVelModelZAxisTransform();
    FixedString			getZDomain() const;

    void			setZRangeCB(CallBacker*);

    VelocityStretcher*		transform_;
    BufferString		selname_;
    MultiID			selkey_;

    uiZRangeInput*		rangefld_;

    uiVelSel*			velsel_;
};


mExpClass(uiSeis) uiTime2Depth : public uiVelModelZAxisTransform
{ mODTextTranslationClass(uiTime2Depth);
public:
    static void			initClass();
    static uiZAxisTransform*	createInstance(uiParent*,
					       const char*,const char*);

				uiTime2Depth(uiParent*);
};


mExpClass(uiSeis) uiDepth2Time : public uiVelModelZAxisTransform
{ mODTextTranslationClass(uiDepth2Time);
public:
    static void			initClass();
    static uiZAxisTransform*	createInstance(uiParent*,
					       const char*,const char*);

				uiDepth2Time(uiParent*);
};





#endif


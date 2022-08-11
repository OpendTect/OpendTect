#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2007
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uitime2depthzaxistrans.h"
#include "veldesc.h"
#include "timedepthconv.h"

class uiSeisSel;
class uiStaticsDesc;
class uiZRangeInput;
class uiPushButton;
class VelocityStretcher;

/*!Group that allows the user to edit VelocityDesc information. */

mExpClass(uiSeis) uiVelocityDesc : public uiGroup
{ mODTextTranslationClass(uiVelocityDesc);
public:

    mExpClass(uiSeis) Setup
    {
    public:
				Setup( const VelocityDesc* vd=0 )
				    : is2d_(false)
				{ if ( vd ) desc_ = *vd; }

	mDefSetupMemb(VelocityDesc,desc)
	mDefSetupMemb(bool,is2d)
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

   bool			acceptOK(CallBacker*) override;
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

    void			setIs2D(bool);

protected:

    void			fillDefault() override;

    void			selectionDoneCB(CallBacker*);
    void			updateEditButton();
    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
    Interval<float>		trg_;
    Interval<float>		brg_;
};


mExpClass(uiSeis) uiVelModelZAxisTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiVelModelZAxisTransform);
public:
    void			enableTargetSampling() override;
    bool			acceptOK() override;

    ZAxisTransform*		getSelection() override;

    const char*			selName() const;
    const MultiID&		selID() const { return selkey_; }

    bool			canBeField() const override { return true; }
protected:
				uiVelModelZAxisTransform(uiParent*,bool);
				~uiVelModelZAxisTransform();

    StringView			getZDomain() const;
    void			finalizeCB(CallBacker*);
    void			setZRangeCB(CallBacker*);

    VelocityStretcher*		transform_;
    BufferString		selname_;
    MultiID			selkey_;

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






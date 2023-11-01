#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "uigeninput.h"
#include "uiseissel.h"
#include "uitime2depthzaxistrans.h"

#include "timedepthconv.h"
#include "veldesc.h"

class UnitOfMeasure;
class uiPushButton;
class uiSeisSel;
class uiStaticsDesc;
class uiUnitSel;
class uiZRangeInput;

/*!Group that allows the user to edit VelocityDesc information. */

mExpClass(uiSeis) uiVelocityDesc : public uiGroup
{ mODTextTranslationClass(uiVelocityDesc);
public:

    mExpClass(uiSeis) Setup
    {
    public:
				Setup(const IOObj*,bool is2d,
				      bool onlyvelocity=true);
				mDeprecated("Use IOObj")
				Setup( const VelocityDesc* vd=0 )
				    : is2d_(false)
				{ if ( vd ) desc_ = *vd; }
				~Setup()
				{ removeParam(); }

	mDefSetupMemb(VelocityDesc,desc)
	mDefSetupMemb(bool,is2d)
	bool			getOnlyVelocity() const;
	Setup&			onlyvelocity(bool yn);

    private:
	void			removeParam();
    };

				uiVelocityDesc(uiParent*,const Setup&);
				mDeprecated("Provide setup")
				uiVelocityDesc(uiParent*,const Setup* s=0);
				~uiVelocityDesc();

    bool			isVelocity() const;
    bool			get(VelocityDesc&,bool displayerrors) const;
    void			set(const VelocityDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

    NotifierAccess&		typeChangeNotifier();

protected:

    void			initGrpCB(CallBacker*);
    void			typeChgCB(CallBacker*);
    void			unitCheckCB(CallBacker*);
    void			hasStaticsChgCB(CallBacker*);
    mDeprecatedDef
    void			updateFlds(CallBacker*);

    OD::VelocityType		getType() const;
    void			setType(OD::VelocityType);

    Setup&			vsu_();
    const EnumDefImpl<OD::VelocityType>& veltypedef_() const;
    uiGenInput*			typefld_;
    uiCheckBox*			unitchkfld_();
    const uiUnitSel*		unitfld_() const;
    uiUnitSel*			unitfld_();
    uiGenInput*			hasstaticsfld_;
    uiStaticsDesc*		staticsfld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */

mExpClass(uiSeis) uiVelocityDescDlg : public uiDialog
{ mODTextTranslationClass(uiVelocityDescDlg)
public:
			uiVelocityDescDlg(uiParent*,const IOObj*,
					  const uiVelocityDesc::Setup&);
			mDeprecated("Provide setup")
			uiVelocityDescDlg(uiParent*,const IOObj* cursel=0,
					  const uiVelocityDesc::Setup* s=0);
			~uiVelocityDescDlg();

    MultiID		getSelection_() const;
    mDeprecated("Use getSelection_")
    IOObj*		getSelection() const;
			//!<returned object must be managed by caller
    bool		isVelocity() const;
    Interval<float>	getVelocityTopRange() const
			{ return toprange_; }
    Interval<float>	getVelocityBottomRange() const
			{ return bottomrange_; }

    const UnitOfMeasure* velUnit() const;
    static bool		doScanVels(const IOObj&,const VelocityDesc&,
				   bool writergs,Interval<float>& topvelrg,
				   Interval<float>& botvelrg,
				   TaskRunner* =nullptr);

protected:

    void		initDlgCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		volSelChange(CallBacker*);
    bool		scanAvgVel(const IOObj&,const VelocityDesc&);

    Interval<float>	toprange_;
    Interval<float>	bottomrange_;

    VelocityDesc	oldveldesc_;
    uiSeisSel*		volselfld_;
    uiVelocityDesc*	veldescfld_;

};


//!Field that selects a velocity volume, and edit it's properties/velocity tag

mExpClass(uiSeis) uiVelSel : public uiSeisSel
{ mODTextTranslationClass(uiVelSel)
public:
				uiVelSel(uiParent*,const uiString&
					     =VelocityDesc::getVelVolumeLabel(),
					     bool is2d=false,
					     bool enabotherdomain=false);
				uiVelSel(uiParent*,const IOObjContext&,
					 const uiSeisSel::Setup&,
					 bool iseditbutton=true);
				~uiVelSel();

    void			setInput(const MultiID&);
    void			setInput_(const IOObj&);
    void			setInput_(const MultiID&);
    void			setVelocityOnly(bool yn);
    static const IOObjContext&	ioContext(bool is2d);
    mDeprecated("Provide is2d flag")
    static const IOObjContext&	ioContext();

    uiRetVal			isOK() const;
    uiRetVal			get(VelocityDesc&,
				    const ZDomain::Info** =nullptr) const;
    Interval<float>		getVelocityTopRange() const	{ return trg_; }
    Interval<float>		getVelocityBottomRange() const	{ return brg_; }

    const UnitOfMeasure*	velUnit() const;

    Notifier<uiVelSel>		velrgchanged;
    Notifier<uiVelSel>&		velChanged()	{ return velrgchanged; }

    mDeprecated("2D flag should be provided on construction")
    void			setIs2D(bool);

protected:

    void			fillDefault() override;
    bool			isOnlyVelocity() const;

    void			initGrpCB(CallBacker*);
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
				uiVelModelZAxisTransform(uiParent*,bool t2d);
				~uiVelModelZAxisTransform();

    StringView			getZDomain() const;
    void			finalizeCB(CallBacker*);
    void			setZRangeCB(CallBacker*);

    VelocityStretcher*		transform_; // Deprecated
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
				~uiTime2Depth();
};


mExpClass(uiSeis) uiDepth2Time : public uiVelModelZAxisTransform
{ mODTextTranslationClass(uiDepth2Time);
public:
    static void			initClass();
    static uiZAxisTransform*	createInstance(uiParent*,
					       const char*,const char*);

				uiDepth2Time(uiParent*);
				~uiDepth2Time();
};

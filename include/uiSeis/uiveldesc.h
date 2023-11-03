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
				~Setup();

	mDefSetupMemb(VelocityDesc,desc)
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,onlyvelocity)
    };

				uiVelocityDesc(uiParent*,const Setup&);
				~uiVelocityDesc();

    void			set(const VelocityDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

    bool			isVelocity() const;
    bool			get(VelocityDesc&,bool displayerrors) const;

    NotifierAccess&		typeChangeNotifier();
    Notifier<uiVelocityDesc>	unitChanged;

private:

    void			initGrpCB(CallBacker*);
    void			typeChgCB(CallBacker*);
    void			unitCheckCB(CallBacker*);
    void			hasStaticsChgCB(CallBacker*);

    Vel::Type			getType() const;
    void			setType(Vel::Type);

    Setup			vsu_;
    EnumDefImpl<Vel::Type>	veltypedef_;
    uiGenInput*			typefld_;
    uiCheckBox*			unitchkfld_;
    uiUnitSel*			unitfld_;
    bool			wasguessed_ = false;
    uiGenInput*			hasstaticsfld_;
    uiStaticsDesc*		staticsfld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */

mExpClass(uiSeis) uiVelocityDescDlg : public uiDialog
{ mODTextTranslationClass(uiVelocityDescDlg)
public:
			uiVelocityDescDlg(uiParent*,const IOObj*,
					  const uiVelocityDesc::Setup&);
			~uiVelocityDescDlg();

    MultiID		getSelection() const;
    bool		isVelocity() const;
    Interval<float>	getVelocityTopRange() const
			{ return toprange_; }
    Interval<float>	getVelocityBottomRange() const
			{ return bottomrange_; }

    const UnitOfMeasure* getVelUnit() const;

protected:

    void		initDlgCB(CallBacker*);
    void		volSelChange(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    bool		scanAvgVel(const IOObj&, const VelocityDesc&);

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
					 const uiSeisSel::Setup&);
				~uiVelSel();

    void			setInput(const IOObj&) override;
    void			setInput(const MultiID&) override;
    void			setVelocityOnly(bool yn);
    static const IOObjContext&	ioContext(bool is2d);

    uiRetVal			isOK() const;
    uiRetVal			get(VelocityDesc&,
				    const ZDomain::Info** =nullptr) const;
    Interval<float>		getVelocityTopRange() const	{ return trg_; }
    Interval<float>		getVelocityBottomRange() const	{ return brg_; }

    const UnitOfMeasure*	getVelUnit() const;

    Notifier<uiVelSel>		velrgchanged;

private:

    void			fillDefault() override;

    void			initGrpCB(CallBacker*);
    void			selectionDoneCB(CallBacker*);
    void			updateEditButton();
    void			editCB(CallBacker*);

    uiPushButton*		editcubebutt_;
    bool			onlyvelocity_ = true;
    Interval<float>		trg_;
    Interval<float>		brg_;
};


mExpClass(uiSeis) uiVelModelZAxisTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiVelModelZAxisTransform);
protected:
				uiVelModelZAxisTransform(uiParent*,bool t2d);
				~uiVelModelZAxisTransform();
private:

    ZAxisTransform*		getSelection() override;
    bool			canBeField() const override { return true; }

    const char*			selName() const;
    const MultiID&		selID() const { return selkey_; }

    StringView			getZDomain() const;

    void			doInitGrp() override;
    void			setZRangeCB(CallBacker*);
    bool			acceptOK() override;

    RefMan<ZAxisTransform>	transform_;
    BufferString		selname_;
    MultiID			selkey_;

    uiVelSel*			velsel_;
};


mExpClass(uiSeis) uiTime2Depth : public uiVelModelZAxisTransform
{ mODTextTranslationClass(uiTime2Depth);
private:
				uiTime2Depth(uiParent*);
				~uiTime2Depth();

    static uiZAxisTransform*	createInstance(uiParent*,
					       const char*,const char*);
public:
    static void			initClass();
};


mExpClass(uiSeis) uiDepth2Time : public uiVelModelZAxisTransform
{ mODTextTranslationClass(uiDepth2Time);
private:
				uiDepth2Time(uiParent*);
				~uiDepth2Time();

    static uiZAxisTransform*	createInstance(uiParent*,
					       const char*,const char*);
public:
    static void			initClass();
};

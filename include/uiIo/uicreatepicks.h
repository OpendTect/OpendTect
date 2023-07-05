#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "datapointset.h"
#include "enums.h"
#include "trckeysampling.h"
#include "uigeninput.h"

class EnumDef;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiLabeledComboBox;
class uiListBox;
class uiPosFilterSetSel;
class uiPosProvider;
class uiPosSubSel;
namespace Pick { class Set; }

/*! \brief Dialog for creating (a) pick set(s) */

mExpClass(uiIo) RandLocGenPars
{ mODTextTranslationClass(RandLocGenPars);
public:
			RandLocGenPars();
			~RandLocGenPars();

    int			nr_		= 1;
    bool		needhor_	= false;
    TrcKeySampling	hs_;
    Interval<float>	zrg_;
    int			horidx_		= -1;
    int			horidx2_	= -1;
    BufferStringSet	linenms_;
};


mExpClass(uiIo) uiCreatePicks : public uiDialog
{
mODTextTranslationClass(uiCreatePicks)
public:
			uiCreatePicks(uiParent*,bool aspolygon=false,
				   bool addstdfields=true, bool zvalreq=false);
    virtual		~uiCreatePicks();

    enum DepthType	{ Feet=0, Meter };
			mDeclareEnumUtils(DepthType);
    enum TimeType	{ Seconds=0, MilliSeconds, MicroSeconds };
			mDeclareEnumUtils(TimeType);

    virtual RefMan<Pick::Set>	getPickSet() const;
    const char*		pickSetName() const {return name_; }
    float		getZVal() { return zvalfld_->getFValue(); }
    float		getZValInSurvUnit() { return zval_; }
    DepthType		getDepthZValType() { return zdepthvaltyp_; }
    TimeType		getTimeZValType()  { return ztimevaltyp_; }

protected:

    uiGenInput*		nmfld_;
    uiColorInput*	colsel_;
    uiGenInput*		zvalfld_;
    uiComboBox*		zvaltypfld_;
    BufferString	name_;

    bool		calcZValAccToSurvDepth();

    bool		aspolygon_;

    bool		acceptOK(CallBacker*) override;
    virtual void	addStdFields(uiObject* lastobj=0);
    bool		iszvalreq_;
    float		zval_;
    DepthType		zdepthvaltyp_;
    TimeType		ztimevaltyp_;
};


mExpClass(uiIo) uiGenPosPicks : public uiCreatePicks
{ mODTextTranslationClass(uiGenPosPicks);
public:
			uiGenPosPicks(uiParent*);
			~uiGenPosPicks();

    RefMan<Pick::Set>	getPickSet() const override;

protected:

    uiPosProvider*	posprovfld_;
    uiGenInput*		maxnrpickfld_;
    uiPosFilterSetSel*	posfiltfld_;
    RefMan<DataPointSet> dps_;

    bool		acceptOK(CallBacker*) override;
    void		posProvChgCB(CallBacker*);
};


mExpClass(uiIo) uiGenRandPicks2D : public uiCreatePicks
{ mODTextTranslationClass(uiGenRandPicks2D);
public:

			uiGenRandPicks2D(uiParent*,const BufferStringSet&,
					 const BufferStringSet&);
			~uiGenRandPicks2D();

    const RandLocGenPars& randPars() const	{ return randpars_; }

protected:

    RandLocGenPars		randpars_;
    const BufferStringSet&	hornms_;

    uiGenInput*		nrfld_;
    uiGenInput*		geomfld_;
    uiLabeledComboBox*	horselfld_;
    uiComboBox*		horsel2fld_;
    uiListBox*		linenmfld_;
    uiGenInput*		zfld_;

    BufferStringSet	linenms_;

    bool		acceptOK(CallBacker*) override;
    void		mkRandPars();

    void		geomSel(CallBacker*);
    void		hor1Sel(CallBacker*);
    void		hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);
};

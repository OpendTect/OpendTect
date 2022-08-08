#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiposprovgroup.h"

class CtxtIOObj;
class TrcKeyZSampling;
class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;
class uiSelSteps;
class uiSelHRange;
class uiSelZRange;
class uiSelNrRange;
class uiIOFileSelect;


/*! \brief UI for RangePosProvider */

mExpClass(uiIo) uiRangePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiRangePosProvGroup)
public:

			uiRangePosProvGroup(uiParent*,
					    const uiPosProvGroup::Setup&);
			~uiRangePosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;
    bool		hasRandomSampling() const override;

    void		setExtractionDefaults() override;

    void		getTrcKeyZSampling(TrcKeyZSampling&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSelHRange*	hrgfld_;
    uiSelZRange*	zrgfld_;
    uiSelNrRange*	nrrgfld_;
    uiGenInput*		samplingfld_;
    uiGenInput*		nrsamplesfld_;

    uiPosProvGroup::Setup setup_;

    void		initGrp(CallBacker*);
    void		samplingCB(CallBacker*);
    void		rangeChgCB(CallBacker*);

};


/*! \brief UI for PolyPosProvider */

mExpClass(uiIo) uiPolyPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiPolyPosProvGroup)
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiPolyPosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		setExtractionDefaults() override;

    bool		getID(MultiID&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();

protected:

    void		inoutCB(CallBacker*);

    CtxtIOObj&		ctio_;

    uiIOObjSel*		polyfld_;
    uiSelSteps*		stepfld_;
    uiSelZRange*	zrgfld_;
    uiGenInput*     inoutfld_;
    uiPosSubSel*    bboxfld_;

};


/*! \brief UI for TablePosProvider */

mExpClass(uiIo) uiTablePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiTablePosProvGroup)
public:
			uiTablePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&,
					   bool onlypointset=true);
			~uiTablePosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    bool		getID(MultiID&) const;
    bool		getFileName(BufferString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiTablePosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio_;

    uiGenInput*		selfld_;
    uiIOObjSel*		psfld_;
    uiIOFileSelect*	tffld_;

    void		selChg(CallBacker*);

};



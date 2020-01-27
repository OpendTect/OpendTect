#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiposprovgroup.h"

class uiFileSel;
class uiGenInput;
class uiIOObjSel;
class uiPickSetIOObjSel;
class uiPosSubSel;
class uiSelSteps;
class uiSelSublineSet;
class uiSelSubvol;
class uiSelZRange;


/*! \brief UI for RangePosProvider */

mExpClass(uiIo) uiRangePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiRangePosProvGroup)
public:

			uiRangePosProvGroup(uiParent*,
					    const uiPosProvGroup::Setup&);
			~uiRangePosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(uiString&) const;
    virtual bool	hasRandomSampling() const;

    void		setExtractionDefaults();

    void		getSubSel(Survey::FullSubSel&) const;

    CNotifier<uiRangePosProvGroup,Pos::GeomID>	geomChanged;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSelSubvol*	volrgfld_	= nullptr;
    uiSelSublineSet*	linergsfld_	= nullptr;
    uiGenInput*		samplingfld_	= nullptr;
    uiGenInput*		nrsamplesfld_	= nullptr;

    uiPosProvGroup::Setup setup_;

    void		initGrp(CallBacker*);
    void		rangeChgCB(CallBacker*);
    void		lineChgCB(CallBacker*);
    void		samplingCB(CallBacker*);

private:
    static void		getLineNames(const Survey::FullSubSel&,
				     BufferStringSet&);
};


/*! \brief UI for PolyPosProvider */

mExpClass(uiIo) uiPolyPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiPolyPosProvGroup)
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiPolyPosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(uiString&) const;

    void		setExtractionDefaults();

    bool		getID(DBKey&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();

protected:

    void		inoutCB(CallBacker*);

    uiPickSetIOObjSel*	polyfld_;
    uiSelSteps*		stepfld_;
    uiSelZRange*	zrgfld_;
    uiGenInput*		inoutfld_;
    uiPosSubSel*	bboxfld_;

};


/*! \brief UI for TablePosProvider */

mExpClass(uiIo) uiTablePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiTablePosProvGroup)
public:
			uiTablePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(uiString&) const;

    bool		getID(DBKey&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiTablePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiGenInput*		typfld_;
    uiPickSetIOObjSel*	psfld_;
    uiIOObjSel*		pvdsfld_;

    void		typSelCB(CallBacker*);
    void		initGrpCB(CallBacker*);

};

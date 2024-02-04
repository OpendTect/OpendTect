#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "seisselection.h"
#include "uigroup.h"
#include "ranges.h"
#include "uistring.h"

class IOObj;
class TrcKeySampling;
class TrcKeyZSampling;

class uiCompoundParSel;
class uiCheckBox;
class uiLineSel;
class uiPosSubSel;
class uiSeis2DSubSel;
class uiSelSubline;
class uiSeis2DMultiLineSel;
class uiSeis2DLineNameSel;

namespace ZDomain { class Info; }


mExpClass(uiSeis) uiSeisSubSel : public uiGroup
{ mODTextTranslationClass(uiSeisSubSel);
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel();

    bool		isAll() const;
    virtual bool	is2D() const					= 0;
    void		getSampling(TrcKeyZSampling&) const;
    void		getSampling(TrcKeySampling&) const;
    void		getZRange(StepInterval<float>&) const;
    const ZDomain::Info* zDomain() const;

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    virtual void	clear();
    virtual void	setInput(const IOObj&)				= 0;
    void		setInput(const TrcKeySampling&);
    void		setInput(const MultiID&);
    void		setInput(const StepInterval<float>& zrg);
    void		setInput(const TrcKeyZSampling&);
    void		setInputLimit(const TrcKeyZSampling&);

    virtual int		expectedNrSamples() const;
    virtual int		expectedNrTraces() const;

    virtual uiCompoundParSel*	compoundParSel();
    Notifier<uiSeisSubSel>	selChange;

protected:

			uiSeisSubSel(uiParent*,const Seis::SelSetup&);

    void		selChangeCB(CallBacker*);
    void		afterSurveyChangedCB(CallBacker*);
    uiPosSubSel*	selfld_;

};


mExpClass(uiSeis) uiSeis3DSubSel : public uiSeisSubSel
{ mODTextTranslationClass(uiSeis3DSubSel);
public:
			uiSeis3DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis3DSubSel();

    bool		is2D() const override	{ return false; }

    void		setInput(const IOObj&) override;
};


mExpClass(uiSeis) uiSeis2DSubSel : public uiSeisSubSel
{ mODTextTranslationClass(uiSeis2DSubSel);
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    bool		is2D() const override	{ return true; }

    void		clear() override;
    bool		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;
    void		setInput(const IOObj&) override;
    void		setInputLines(const TypeSet<Pos::GeomID>&);

    bool		isSingLine() const;
    BufferString	selectedLine() const;
    void		setSelectedLine(const char*);

    void		selectedGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		selectedLines(BufferStringSet&) const;
    void		setSelectedLines(const BufferStringSet&);

    int			expectedNrSamples() const override;
    int			expectedNrTraces() const override;

    void		getSampling(TrcKeyZSampling&,Pos::GeomID) const;
    StepInterval<int>	getTrcRange(Pos::GeomID) const;
    StepInterval<float> getZRange(Pos::GeomID) const;

protected:

    void		lineChg(CallBacker*);

    uiSeis2DMultiLineSel* multilnmsel_	= nullptr;
    uiSeis2DLineNameSel* singlelnmsel_	= nullptr;

    MultiID		inpkey_;

};



mExpClass(uiSeis) uiMultiZSeisSubSel : public uiGroup
{
public:
			uiMultiZSeisSubSel(uiParent*,const Seis::SelSetup&,
			      const ObjectSet<const ZDomain::Info>* =nullptr);
			~uiMultiZSeisSubSel();

    bool		setInput(const MultiID&);
    bool		setInput(const IOObj&);
    bool		setZDomain(const ZDomain::Info&);
    bool		setInput(const TrcKeyZSampling&,
				 const ZDomain::Info* =nullptr);
    bool		setInputLimit(const TrcKeyZSampling&,
				      const ZDomain::Info* =nullptr);

    bool		getSampling(TrcKeyZSampling&) const;
    const ZDomain::Info* zDomain() const;

    const uiSeisSubSel* getSelGrp() const;
    uiSeisSubSel*	getSelGrp();

    Notifier<uiMultiZSeisSubSel> selChange;

private:

    void		initGrpCB(CallBacker*);
    void		selCB(CallBacker*);

    const uiSeisSubSel* getSelGrp(const ZDomain::Info*) const;
    uiSeisSubSel*	getSelGrp(const ZDomain::Info*);

    ObjectSet<uiSeisSubSel> subselflds_;
};

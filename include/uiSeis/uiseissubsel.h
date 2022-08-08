#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "bufstringset.h"
#include "multiid.h"
#include "seisselection.h"
#include "uidialog.h"
#include "uigroup.h"
#include "ranges.h"
#include "sets.h"
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


mExpClass(uiSeis) uiSeisSubSel : public uiGroup
{ mODTextTranslationClass(uiSeisSubSel);
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel();

    bool		isAll() const;
    void		getSampling(TrcKeyZSampling&) const;
    void		getSampling(TrcKeySampling&) const;
    void		getZRange(StepInterval<float>&) const;

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

			uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
			    : uiSeisSubSel(p,ss)		{}

    void		setInput(const IOObj&) override;

};


mExpClass(uiSeis) uiSeis2DSubSel : public uiSeisSubSel
{ mODTextTranslationClass(uiSeis2DSubSel);
public:

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    void		clear() override;
    bool		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;
    void		setInput(const IOObj&) override;
    void		setInputLines(const TypeSet<Pos::GeomID>&);

    bool		isSingLine() const;
    const char*		selectedLine() const;
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

    uiSeis2DMultiLineSel*	multilnmsel_;
    uiSeis2DLineNameSel*	singlelnmsel_;

    bool		multiln_;
    MultiID		inpkey_;

    void		lineChg(CallBacker*);
};


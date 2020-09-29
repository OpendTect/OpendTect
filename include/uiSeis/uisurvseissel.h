#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2001
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uisurvioobjsel.h"
#include "uisurvioobjselgrp.h"
#include "seistype.h"
class uiSurvSeisSelGroupCompEntry;


mExpClass(uiSeis) uiSurvSeisSel : public uiSurvIOObjSel
{
public:

    typedef Seis::GeomType   GeomType;

    mExpClass(uiSeis) Setup
    {
    public:
			Setup( bool fixedsurv, GeomType gt=Seis::Vol )
			    : fixedsurvey_(fixedsurv)
			    , geomtype_(gt)
			    , lbltxt_(toUiString("-"))	//!< default no label
			    , steerflag_(0)		{}


	mDefSetupMemb(bool,	fixedsurvey)
	mDefSetupMemb(GeomType,	geomtype)
	mDefSetupMemb(uiString,	lbltxt)	    //!< set empty to 'generate' one
	mDefSetupMemb(int,	steerflag)  //!< 0=don't care, 1=only, -1=none

    };

			uiSurvSeisSel(uiParent*,const Setup&);
			~uiSurvSeisSel();

    inline const Setup&	setup() const	{ return setup_; }

    void		setCompNr(int);
    int			compNr() const;
    int			nrComps() const;
    const char*		compName(int) const;

    Notifier<uiSurvSeisSel> compSel;

protected:

    Setup		setup_;

    uiComboBox*		compfld_;

    void		initSeisGrp(CallBacker*);
    void		inpSelChgCB(CallBacker*);
    void		compSelCB(CallBacker*);

    void		updateComps();

};


mExpClass(uiSeis) uiSurvSeisSelGroup : public uiSurvIOObjSelGroup
{
public:

    typedef Seis::GeomType		GeomType;
    typedef uiSurvSeisSel::Setup	Setup;

			uiSurvSeisSelGroup(uiParent*,const Setup&,
				       bool selmulti=false,bool fixsurv=false,
				       bool withinserters=false);
			~uiSurvSeisSelGroup();

    void		setSelected(const DBKey&,int compnr, bool add=false);

    virtual bool	evaluateInput();

    // Available after evaluateInput():
    int			nrComps(int iselected=0) const;
    bool		isSelectedComp(int icomp,int iselected=0) const;
    const char*		compName(int icomp,int iselected=0) const;

protected:

    Setup		setup_;
    uiListBox*		compfld_;
    mutable ObjectSet<uiSurvSeisSelGroupCompEntry>  compentries_;
    int			prevselidx_		= -1;

    void		initSeisGrp(CallBacker*);
    void		seisSelChgCB(CallBacker*);
    void		refresh();

    uiSurvSeisSelGroupCompEntry& getCompEntry(int,bool selected) const;

};

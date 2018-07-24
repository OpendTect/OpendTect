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


mExpClass(uiSeis) uiSurvSeisSel : public uiSurvIOObjSel
{
public:

    typedef Seis::GeomType   GeomType;
    typedef Seis::DataType   DataType;

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

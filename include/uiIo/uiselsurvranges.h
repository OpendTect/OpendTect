#ifndef uiselsurvranges_h
#define uiselsurvranges_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiselsurvranges.h,v 1.2 2008-02-18 11:00:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "cubesampling.h"
class uiSpinBox;
class uiLineEdit;

/*!\brief Selects sub-Z-range. Default will be SI() work Z Range. */

class uiSelZRange : public uiGroup
{
public:
                        uiSelZRange(uiParent*,bool wstep);

    StepInterval<float>	getRange() const;
    void		setRange(const StepInterval<float>&);

protected:

    uiSpinBox*		startfld_;
    uiSpinBox*		stopfld_;
    uiSpinBox*		stepfld_;

    void		valChg(CallBacker*);

};


/*!\brief Selects sub-volume. Default will be SI() work area */

class uiSelNrRange : public uiGroup
{
public:
    enum Type		{ Inl, Crl, Gen };

                        uiSelNrRange(uiParent*,Type,bool wstep);

    StepInterval<int>	getRange() const;
    void		setRange(const StepInterval<int>&);

protected:

    uiSpinBox*		startfld_;
    uiLineEdit*		stopfld_;
    uiSpinBox*		stepfld_;
    int			defstep_;

    void		valChg(CallBacker*);

};


/*!\brief Selects sub-volume. Default will be SI() work area */

class uiSelHRange : public uiGroup
{
public:
                        uiSelHRange(uiParent*,bool wstep);

    HorSampling		getSampling() const;
    void		setSampling(const HorSampling&);

    uiSelNrRange*	inlfld_;
    uiSelNrRange*	crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

class uiSelSubvol : public uiGroup
{
public:
                        uiSelSubvol(uiParent*,bool wstep);

    CubeSampling	getSampling() const;
    void		setSampling(const CubeSampling&);

    uiSelHRange*	hfld_;
    uiSelZRange*	zfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

class uiSelSubline : public uiGroup
{
public:
                        uiSelSubline(uiParent*,bool wstep);

    uiSelNrRange*	nrfld_;
    uiSelZRange*	zfld_;

};


#endif

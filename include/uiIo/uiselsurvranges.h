#ifndef uiselsurvranges_h
#define uiselsurvranges_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiselsurvranges.h,v 1.12 2009-03-27 15:52:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "cubesampling.h"
class uiSpinBox;
class uiLineEdit;

/*!\brief Selects sub-Z-range. Default will be SI() work Z Range. */

mClass uiSelZRange : public uiGroup
{
public:
                        uiSelZRange(uiParent*,bool wstep,
				    bool isrel=false,const char* lbltxt=0);
			uiSelZRange(uiParent* p,StepInterval<float> limitrg,
				    bool wstep,const char* lbltxt=0);

    StepInterval<float>	getRange() const;
    void		setRange(const StepInterval<float>&);

protected:

    uiSpinBox*		startfld_;
    uiSpinBox*		stopfld_;
    uiSpinBox*		stepfld_;
    bool		isrel_;

    void		valChg(CallBacker*);
    void		makeInpFields(const char*,bool,StepInterval<float>);

};


/*!\brief Selects range of trace numbers */

mClass uiSelNrRange : public uiGroup
{
public:
    enum Type		{ Inl, Crl, Gen };

                        uiSelNrRange(uiParent*,Type,bool wstep);
			uiSelNrRange(uiParent*,StepInterval<int> limit,
				     bool wstep,const char*);

    StepInterval<int>	getRange() const;
    void		setRange(const StepInterval<int>&);
    void		setLimitRange(const StepInterval<int>&);

    Notifier<uiSelNrRange>	rangeChanged;

protected:

    uiSpinBox*		startfld_;
    uiSpinBox*		icstopfld_;
    uiLineEdit*		nrstopfld_;
    uiSpinBox*		stepfld_;
    int			defstep_;

    void		valChg(CallBacker*);

    int			getStopVal() const;
    void		setStopVal(int);
    void		makeInpFields(const char*,StepInterval<int>,bool,bool);

};


/*!\brief Selects step(s) in inl/crl or trcnrs */

mClass uiSelSteps : public uiGroup
{
public:

                        uiSelSteps(uiParent*,bool is2d);

    BinID		getSteps() const;
    void		setSteps(const BinID&);

protected:

    uiSpinBox*		inlfld_;
    uiSpinBox*		crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work area */

mClass uiSelHRange : public uiGroup
{
public:
                        uiSelHRange(uiParent*,bool wstep);
                        uiSelHRange(uiParent*,const HorSampling& limiths,
				    bool wstep);

    HorSampling		getSampling() const;
    void		setSampling(const HorSampling&);

    uiSelNrRange*	inlfld_;
    uiSelNrRange*	crlfld_;

};


/*!\brief Selects sub-volume. Default will be SI() work volume */

mClass uiSelSubvol : public uiGroup
{
public:
                        uiSelSubvol(uiParent*,bool wstep);

    CubeSampling	getSampling() const;
    void		setSampling(const CubeSampling&);

    uiSelHRange*	hfld_;
    uiSelZRange*	zfld_;

};


/*!\brief Selects sub-line. Default will be 1-udf and SI() z range */

mClass uiSelSubline : public uiGroup
{
public:
                        uiSelSubline(uiParent*,bool wstep);

    uiSelNrRange*	nrfld_;
    uiSelZRange*	zfld_;

};


#endif

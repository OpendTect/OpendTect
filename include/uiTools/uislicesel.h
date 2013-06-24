#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "cubesampling.h"
#include "ranges.h"
#include "zdomain.h"
#include "threadlock.h"

class uiCheckBox;
class uiLabeledSpinBox;
class uiPushButton;
class uiScrollDialog;
class uiSpinBox;
class uiSliceScroll;


mExpClass(uiTools) uiSliceSel : public uiGroup
{
public:

    enum Type			{ Inl, Crl, Tsl, Vol, TwoD };

				uiSliceSel(uiParent*,Type,const ZDomain::Info&,
					   bool dogeomcheck=true);
				~uiSliceSel();

    void			setApplyCB(const CallBack&);

    const CubeSampling&		getCubeSampling() const	{ return cs_; }
    virtual void		setCubeSampling(const CubeSampling&);
    void			setMaxCubeSampling(const CubeSampling&);
    void			enableApplyButton(bool);
    void			enableScrollButton(bool);
    void			fillPar(IOPar&);
    void			usePar(const IOPar&);

    bool			acceptOK();

protected:

    friend class		uiSliceScroll;

    void			createInlFld();
    void			createCrlFld();
    void			createZFld();

    void			scrollPush(CallBacker*);
    void			applyPush(CallBacker*);
    void			readInput();
    void			updateUI();
    void			setBoxValues(uiSpinBox*,
	    				     const StepInterval<int>&,int);

    uiLabeledSpinBox*           inl0fld_;
    uiLabeledSpinBox*           crl0fld_;
    uiLabeledSpinBox*           z0fld_;
    uiSpinBox*			inl1fld_;
    uiSpinBox*			crl1fld_;
    uiSpinBox*			z1fld_;
    uiButton*			applybut_;
    uiButton*			scrollbut_;

    uiSliceScroll*		scrolldlg_;

    CubeSampling		maxcs_;
    CubeSampling		cs_;
    CallBack*			applycb_;
    bool			isinl_, iscrl_, istsl_, isvol_, is2d_,
				dogeomcheck_;
    ZDomain::Info		zdominfo_;

    Threads::Lock		updatelock_;
};


mExpClass(uiTools) uiSliceSelDlg : public uiDialog
{
public:
    				uiSliceSelDlg(uiParent*,
					      const CubeSampling& csin,
					      const CubeSampling& maxcs,
					      const CallBack& applycb,
					      uiSliceSel::Type,
					      const ZDomain::Info&);

    const CubeSampling&		getCubeSampling() const
    				{ return slicesel_->getCubeSampling(); }
    void			setCubeSampling( const CubeSampling& cs )
				{ slicesel_->setCubeSampling( cs ); }

    uiSliceSel*			grp()	{ return slicesel_; }

protected:

    uiSliceSel*			slicesel_;

    bool			acceptOK(CallBacker*);
};

#endif


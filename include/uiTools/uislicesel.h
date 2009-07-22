#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.20 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "cubesampling.h"
#include "ranges.h"

class uiCheckBox;
class uiLabeledSpinBox;
class uiPushButton;
class uiScrollDialog;
class uiSpinBox;
class uiSliceScroll;

namespace Threads { class Mutex; };


mClass uiSliceSel : public uiGroup
{
public:
    enum Type			{ Inl, Crl, Tsl, Vol, TwoD };

				uiSliceSel(uiParent*,Type);
				~uiSliceSel();

    void			setApplyCB(const CallBack&);

    const CubeSampling&		getCubeSampling() const	{ return cs_; }
    void			setCubeSampling(const CubeSampling&);
    void			setMaxCubeSampling(const CubeSampling&);
    void			enableApplyButton(bool);
    void			enableScrollButton(bool);

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
    bool			isinl_, iscrl_, istsl_, isvol_, is2d_;

    Threads::Mutex&		updatemutex_;
};


mClass uiSliceSelDlg : public uiDialog
{
public:
    				uiSliceSelDlg(uiParent*,
					      const CubeSampling& csin,
					      const CubeSampling& maxcs,
					      const CallBack& applycb,
					      uiSliceSel::Type);

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

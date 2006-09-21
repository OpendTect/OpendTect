#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.15 2006-09-21 15:17:45 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"

class uiCheckBox;
class uiLabeledSpinBox;
class uiPushButton;
class uiScrollDialog;
class uiSpinBox;
class CubeSampling;
class uiSliceScroll;

namespace Threads { class Mutex; };


class uiSliceSel : public uiDialog
{
public:
    enum Type			{ Inl, Crl, Tsl, Vol, TwoD };

				uiSliceSel(uiParent*,const CubeSampling& csin,
					   const CubeSampling& maxcs,
					   const CallBack& applycb,Type);
				~uiSliceSel();
    const CubeSampling&		getCubeSampling()	{ return cs_; }
    void			setCubeSampling(const CubeSampling&);
    void			disableApplyButton();
    void			disableScrollButton();

protected:

    friend class		uiSliceScroll;

    void			scrollPush(CallBacker*);
    void			applyPush(CallBacker*);
    void			readInput();
    bool			acceptOK(CallBacker*);
    void			setBoxValues(uiSpinBox*,
	    				     const StepInterval<int>&,int);
    void			createInlFld();
    void			createCrlFld();
    void			createZFld();

    uiLabeledSpinBox*           inl0fld;
    uiLabeledSpinBox*           crl0fld;
    uiLabeledSpinBox*           z0fld;
    uiSpinBox*			inl1fld;
    uiSpinBox*			crl1fld;
    uiSpinBox*			z1fld;
    uiButton*			applybut_;
    uiButton*			scrollbut_;

    uiSliceScroll*		scrolldlg_;

    const CubeSampling&		maxcs_;

    CubeSampling&		cs_;
    CallBack&			applycb_;
    bool			isinl_, iscrl_, istsl_, isvol_, is2d_;

    Threads::Mutex&		updatemutex_;
};

#endif

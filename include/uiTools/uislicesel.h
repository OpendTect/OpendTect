#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.13 2006-03-01 12:33:43 cvsnanne Exp $
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

    uiSliceScroll*		scrolldlg_;

    const CubeSampling&		curcs_;
    const CubeSampling&		maxcs_;

    CubeSampling&		cs_;
    CallBack&			applycb_;
    bool			isinl_, iscrl_, istsl_, isvol_, is2d_;

    Threads::Mutex&		updatemutex_;
};

#endif

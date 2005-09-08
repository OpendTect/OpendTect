#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.11 2005-09-08 10:45:10 cvsnanne Exp $
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

namespace Threads { class Mutex; };


class uiSliceSel : public uiDialog
{
public:
    enum Type			{ Inl, Crl, Tsl, Vol, TwoD };

				uiSliceSel(uiParent*,const CubeSampling&,
					   const CubeSampling&,
					   const CallBack&,Type);
				~uiSliceSel();
    const CubeSampling&		getCubeSampling()	{ return cs; }

protected:

    friend class		uiSliceScroll;

    void			scrollPush(CallBacker*);
    void			applyPush(CallBacker*);
    void			readInput();
    bool			acceptOK(CallBacker*);
    void			setBoxValues(uiSpinBox*,
	    				     const StepInterval<int>&, int );
    void			createInlFld();
    void			createCrlFld();
    void			createZFld();

    uiLabeledSpinBox*           inl0fld;
    uiLabeledSpinBox*           crl0fld;
    uiLabeledSpinBox*           z0fld;
    uiSpinBox*			inl1fld;
    uiSpinBox*			crl1fld;
    uiSpinBox*			z1fld;

    uiSliceScroll*		scrolldlg;

    const CubeSampling&		curcs;
    const CubeSampling&		maxcs;

    CubeSampling&		cs;
    CallBack&			appcb;
    bool			isinl, iscrl, istsl, isvol, is2d;

    Threads::Mutex&		updatemutex;
};

#endif

#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.10 2005-04-12 16:26:50 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"

class uiLabeledSpinBox;
class uiSpinBox;
class CubeSampling;
class uiPushButton;
class uiCheckBox;

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

    const CubeSampling&		curcs;
    const CubeSampling&		maxcs;

    CubeSampling&		cs;
    CallBack&			appcb;
    bool			isinl, iscrl, istsl, isvol, is2d;

    Threads::Mutex&		updatemutex;
};

#endif

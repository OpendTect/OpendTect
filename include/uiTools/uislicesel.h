#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.6 2003-11-07 12:21:54 bert Exp $
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
				uiSliceSel(uiParent*,const CubeSampling&,
					   const CallBack&,bool isvol=false);
				~uiSliceSel();
    const CubeSampling&		getCubeSampling()	{ return cs; }

protected:

    void			updateSel(CallBacker*);
    void			csChanged(CallBacker*);
    void			stepSel(CallBacker*);
    void			readInput();
    bool			acceptOK(CallBacker*);
    void			setBoxValues(uiSpinBox*,
	    				     const StepInterval<int>&, int );

    uiLabeledSpinBox*           inl0fld;
    uiLabeledSpinBox*           crl0fld;
    uiLabeledSpinBox*           z0fld;
    uiSpinBox*			inl1fld;
    uiSpinBox*			crl1fld;
    uiSpinBox*			z1fld;
    uiCheckBox*			doupdfld;
    uiLabeledSpinBox*		stepfld;

    CubeSampling&		cs;
    Notifier<uiSliceSel>	cschanged;
    bool			isinl,iscrl,istsl,isvol;

    Threads::Mutex&		updatemutex;
};

#endif

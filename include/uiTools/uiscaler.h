#ifndef uiscaler_h
#define uiscaler_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.h,v 1.6 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class Scaler;
class uiGenInput;
class uiCheckBox;


class uiScaler : public uiGroup
{
public:

			uiScaler(uiParent*,const char* txt=0, // "Scale values"
				 bool linear_only=false);

    Scaler*		getScaler() const;
    void		setInput(const Scaler&);
    void		setUnscaled();

protected:

    uiCheckBox*		ynfld;
    uiGenInput*		typefld;
    uiGenInput*		linearfld;
    uiGenInput*		basefld;

    void		doFinalise(CallBacker*);
    void		typeSel(CallBacker*);
};


#endif

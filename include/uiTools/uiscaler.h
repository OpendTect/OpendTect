#ifndef uiscaler_h
#define uiscaler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.h,v 1.9 2012-08-03 13:01:15 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
class Scaler;
class uiGenInput;
class uiCheckBox;


mClass(uiTools) uiScaler : public uiGroup
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


#ifndef uiscaler_h
#define uiscaler_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.h,v 1.4 2002-08-02 10:27:23 bert Exp $
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

protected:

    uiCheckBox*		ynfld;
    uiGenInput*		typefld;
    uiGenInput*		linearfld;
    uiGenInput*		basefld;

    void		doFinalise(CallBacker*);
    void		typeSel(CallBacker*);
};


#endif

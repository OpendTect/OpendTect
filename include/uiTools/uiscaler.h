#ifndef uiscaler_h
#define uiscaler_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.h,v 1.2 2002-05-30 22:09:59 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class Scaler;
class uiGenInput;


class uiScaler : public uiGroup
{
public:

			uiScaler(uiParent*,const char* txt=0, //"Value scaling"
				 bool linear_only=false);
			~uiScaler();

    Scaler*		getScaler();

protected:

    Scaler*		scaler;
    uiGenInput*		typefld;
    uiGenInput*		linearfld;
    uiGenInput*		basefld;

    void		doFinalise(CallBacker*);
    void		typeSel(CallBacker*);
};


#endif

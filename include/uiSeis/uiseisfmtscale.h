#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.1 2002-05-24 14:38:21 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiGenInput;
class Scaler;
class IOObj;


class uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*);

    Scaler*		getScaler() const;
    int			getFormat() const;
    			//!< actually returns DataCharacteristics::UserType
    void		updateIOObj(IOObj*) const;

protected:

    uiGenInput*		imptypefld;
    uiGenInput*		scalefld;

};


#endif

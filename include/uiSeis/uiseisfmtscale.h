#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.3 2002-06-26 16:34:41 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiGenInput;
class uiScaler;
class Scaler;
class IOObj;


class uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*,bool with_format=true);
    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    int			getFormat() const;
    			//!< actually returns DataCharacteristics::UserType
    void		updateIOObj(IOObj*) const;

protected:

    uiGenInput*		imptypefld;
    uiScaler*		scalefld;

};


#endif

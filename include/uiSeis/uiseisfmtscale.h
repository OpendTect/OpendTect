#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.6 2003-11-07 12:21:54 bert Exp $
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
    bool		horOptim() const;
    void		updateIOObj(IOObj*) const;

    void		setSteering(bool);

protected:

    uiGenInput*		imptypefld;
    uiGenInput*		optimfld;
    uiScaler*		scalefld;

};


#endif

#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.8 2004-08-26 10:47:45 bert Exp $
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

    bool		is2D() const		{ return is2d; }
    void		set2D(bool);
    bool		isSteer() const		{ return issteer; }
    void		setSteering(bool);

protected:

    uiGenInput*		imptypefld;
    uiGenInput*		optimfld;
    uiScaler*		scalefld;

    bool		is2d;
    bool		issteer;

    void		updFldsForType(CallBacker*);

};


#endif

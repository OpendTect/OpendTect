#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.9 2005-08-15 16:17:07 cvsbert Exp $
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

			uiSeisFmtScale(uiParent*,bool with_format=true,
						 bool prestack=false);
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
    bool		isPS() const		{ return isps; }

protected:

    uiGenInput*		imptypefld;
    uiGenInput*		optimfld;
    uiScaler*		scalefld;

    bool		is2d;
    bool		issteer;
    bool		isps;

    void		updFldsForType(CallBacker*);

};


#endif

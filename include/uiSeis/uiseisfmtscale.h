#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id: uiseisfmtscale.h,v 1.11 2006-12-12 17:48:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "seistype.h"
class uiGenInput;
class uiScaler;
class Scaler;
class IOObj;


class uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*,Seis::GeomType,
				       bool forexport=true);
    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    int			getFormat() const;
    			//!< returns (int)DataCharacteristics::UserType
    bool		horOptim() const;
    void		updateIOObj(IOObj*) const;

    bool		isSteering() const	{ return issteer_; }
    void		setSteering(bool);

protected:

    uiGenInput*		imptypefld;
    uiGenInput*		optimfld;
    uiScaler*		scalefld;

    Seis::GeomType	geom_;
    bool		issteer_;

    void		updSteer(CallBacker*);

};


#endif

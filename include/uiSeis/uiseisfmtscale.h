#ifndef uiseisfmtscale_h
#define uiseisfmtscale_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigroup.h"
#include "seistype.h"
class IOObj;
class Scaler;
class uiScaler;
class uiGenInput;
class uiSeisFmtScaleComp;


mClass(uiSeis) uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*,Seis::GeomType,
				       bool forexport=true,bool withext=true);
    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    int			getFormat() const;
    			//!< returns (int)DataCharacteristics::UserType
    bool		horOptim() const;
    bool		extendTrcToSI() const;
    void		updateIOObj(IOObj*,bool commit=true) const;

    bool		isSteering() const	{ return issteer_; }
    void		setSteering(bool);

protected:

    uiSeisFmtScaleComp*	compfld_;
    uiScaler*		scalefld_;

    Seis::GeomType	gt_;
    bool		issteer_;

    void		updSteer(CallBacker*);
    friend class	uiSeisFmtScaleComp;

};


#endif


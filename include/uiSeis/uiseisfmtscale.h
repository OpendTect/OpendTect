#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigroup.h"
#include "seistype.h"
class IOObj;
class Scaler;
class uiScaler;
class uiSeisFmtScaleComp;


mExpClass(uiSeis) uiSeisFmtScale : public uiGroup
{
public:

			uiSeisFmtScale(uiParent*,Seis::GeomType,
				       bool forexport=true,bool withext=true);
			~uiSeisFmtScale();

    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    int			getFormat() const;
			//!< returns (int)DataCharacteristics::UserType
    bool		horOptim() const;
    bool		extendTrcToSI() const;
    void		updateIOObj(IOObj*,bool commit=true) const;

    bool		isSteering() const	{ return issteer_; }
    void		setSteering(bool);

    void		fillFmtPars(IOPar&) const; // for IOObj
    void		fillOtherPars(IOPar&) const;
    static const char*	sKeyOptDir()		{ return "Optimized direction";}

protected:

    uiSeisFmtScaleComp*	compfld_;
    uiScaler*		scalefld_;

    Seis::GeomType	gt_;
    bool		issteer_;

    void		updSteer(CallBacker*);
    friend class	uiSeisFmtScaleComp;

};

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2002
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
    void		updateFrom(const IOObj&);

    Scaler*		getScaler() const;
    OD::DataRepType	getFormat() const;
    bool		horOptim() const;
    bool		extendTrcToSI() const;
    void		updateIOObj(IOObj*,bool commit=true) const;

    bool		isSteering() const	{ return issteer_; }
    void		setIsSteering(bool);

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

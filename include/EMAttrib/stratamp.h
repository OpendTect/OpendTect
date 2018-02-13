#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		March 2008
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "emposid.h"
#include "trckeysampling.h"
#include "stattype.h"
#include "uistrings.h"

class od_ostream;

namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; class Processor; }
namespace Seis { class Provider; }

mExpClass(EMAttrib) StratAmpCalc  : public Executor
{ mODTextTranslationClass(StratAmpCalc)
public:

			StratAmpCalc(const EM::Horizon3D*,const EM::Horizon3D*,
				     Stats::Type,const TrcKeySampling&,bool);
			~StratAmpCalc();

    int			nextStep();
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		message() const;
    uiString		nrDoneText() const
    { return uiStrings::phrHandled(uiStrings::sPosition(mPlural)); }

    int			init(const IOPar&);
    bool		saveAttribute(const EM::Horizon3D*,int attribidx,
				      bool overwrite,od_ostream* s=0);

    static const char*	sKeyTopHorizonID();
    static const char*	sKeyBottomHorizonID();
    static const char*	sKeySingleHorizonYN();
    static const char*	sKeyAddToTopYN();
    static const char*	sKeyAmplitudeOption();
    static const char*	sKeyTopShift();
    static const char*	sKeyBottomShift();
    static const char*	sKeyOutputFoldYN();
    static const char*	sKeyAttribName();
    static const char*	sKeyIsClassification();
    static const char*	sKeyIsOverwriteYN();

protected:

    Stats::Type			stattyp_;
    Seis::Provider*		prov_;
    bool			usesstored_;
    const EM::Horizon3D*	tophorizon_;
    const EM::Horizon3D*	bothorizon_;
    int				nrdone_;
    int				totnr_;
    float			tophorshift_;
    float			bothorshift_;
    EM::PosID			posid_;
    EM::PosID			posidfold_;
    int				dataidx_;
    int				dataidxfold_;
    bool			addtotop_;
    bool			outfold_;
    bool			isclassification_;
    uiString			errmsg_;
    TrcKeySampling		hs_;
    Attrib::DescSet*		descset_;
    Attrib::Processor*		proc_;

};

#ifndef stratamp_h
#define stratamp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		March 2008
 RCS:		$Id: stratamp.h,v 1.11 2012-08-03 13:01:32 cvskris Exp $
________________________________________________________________________

-*/

#include "horizonattribmod.h"
#include "executor.h"

#include "emposid.h"
#include "horsampling.h"
#include "stattype.h"

class SeisTrcReader;

namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; class Processor; }

mClass(HorizonAttrib) StratAmpCalc  : public Executor
{
public:

    			StratAmpCalc(const EM::Horizon3D*,const EM::Horizon3D*, 
				     Stats::Type,const HorSampling&,bool);
    			~StratAmpCalc();

    int                 nextStep();
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }

    int			init(const IOPar&);
    bool		saveAttribute(const EM::Horizon3D*,int attribidx,
	    			      bool overwrite, std::ostream* strm=0);

    static const char*	sKeyTopHorizonID();
    static const char*	sKeyBottomHorizonID();
    static const char*	sKeySingleHorizonYN();
    static const char*	sKeyAddToTopYN();
    static const char*	sKeyAmplitudeOption();
    static const char*	sKeyTopShift();
    static const char*	sKeyBottomShift();
    static const char*	sKeyOutputFoldYN();
    static const char*	sKeyAttribName();
    static const char*	sKeyIsOverwriteYN();

protected:

    Stats::Type			stattyp_;
    SeisTrcReader*		rdr_;
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
    HorSampling			hs_;
    Attrib::DescSet*		descset_;
    Attrib::Processor*		proc_;
};
#endif


#ifndef stratamp_h
#define stratamp_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara Rao
 Date:		March 2008
 RCS:		$Id: stratamp.h,v 1.8 2011-03-03 13:32:12 cvshelene Exp $
________________________________________________________________________

-*/

#include "executor.h"

#include "emposid.h"
#include "horsampling.h"
#include "stattype.h"

class SeisTrcReader;

namespace EM { class Horizon3D; }
namespace Attrib { class DescSet; class Processor; }

mClass StratAmpCalc  : public Executor
{
public:

    			StratAmpCalc(const EM::Horizon3D*,const EM::Horizon3D*, 
				     Stats::Type,const HorSampling&,bool);
    			~StratAmpCalc();

    void                clear();
    int                 nextStep();
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    const char*		message() const		{ return "Computing..."; }
    const char*		nrDoneText() const	{ return "Points done"; }    

    void		setOffsets( float top, float bot )
			{ tophorshift_ = top; bothorshift_ = bot; }
    int			init(const char* attribnm,bool addtotop,const IOPar&);
    int			getFoldDataIdx() const	{ return dataidxfold_; }

protected:

    Stats::Type		stattyp_;

    SeisTrcReader*      rdr_;
    bool		usesstored_;

    const EM::Horizon3D*      tophorizon_;
    const EM::Horizon3D*      bothorizon_;

    int			nrdone_;
    int			totnr_;

    float		tophorshift_;
    float		bothorshift_;
    EM::PosID           posid_;
    EM::PosID           posidfold_;
    int			dataidx_;
    int			dataidxfold_;
    bool		addtotop_;
    bool		outfold_;

    HorSampling		hs_;

    Attrib::DescSet*	descset_;
    Attrib::Processor*	proc_;
};
#endif

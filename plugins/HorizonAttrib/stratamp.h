#ifndef stratamp_h
#define stratamp_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nageswara Rao
 Date:		March 2008
 RCS:		$Id: stratamp.h,v 1.3 2008-06-04 06:54:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"

#include "emposid.h"
#include "stattype.h"

class HorSampling;
class IOObj;
class SeisTrcReader;

namespace EM { class Horizon3D; }

class StratAmpCalc  : public Executor
{
public:

    			StratAmpCalc(const IOObj&,const EM::Horizon3D*,
				     const EM::Horizon3D*, 
				     Stats::Type,const HorSampling&);
    			~StratAmpCalc();

    void                clear();
    int                 nextStep();
    int			totalNr() const		{ return totnr_; }
    int			nrDone() const		{ return nrdone_; }
    const char*		message() const		{ return "Computing..."; }
    const char*		nrDoneText() const	{ return "Points done"; }    

    void		setOffsets(float top,float bot)
			{ tophorshift_ = top; bothorshift_ = bot; }
    int			init( const char* attribnm , bool addtotop );

protected:

    Stats::Type		stattyp_;

    SeisTrcReader*      rdr_;

    const EM::Horizon3D*      tophorizon_;
    const EM::Horizon3D*      bothorizon_;

    int			nrdone_;
    int			totnr_;

    float		tophorshift_;
    float		bothorshift_;
    EM::PosID           posid_;
    int			dataidx_;
    bool		addtotop_;
};
#endif

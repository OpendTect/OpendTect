#ifndef stratamp_h
#define stratamp_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nageswara Rao
 Date:		March 2008
 RCS:		$Id: stratamp.h,v 1.5 2009-04-06 07:20:09 cvsranojay Exp $
________________________________________________________________________

-*/

#include "executor.h"

#include "emposid.h"
#include "stattype.h"

class HorSampling;
class IOObj;
class SeisTrcReader;

namespace EM { class Horizon3D; }

mClass StratAmpCalc  : public Executor
{
public:

    			StratAmpCalc(const IOObj&,const EM::Horizon3D*,
				     const EM::Horizon3D*, 
				     Stats::Type,const HorSampling&);
    			~StratAmpCalc();

    void                clear();
    int                 nextStep();
    od_int64		totalNr() const		{ return totnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
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

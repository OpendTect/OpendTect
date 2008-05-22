#ifndef stratamp_h
#define stratamp_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Nageswara
 * DATE     : March 2008
 * ID       : $Id: stratamp.h,
-*/

#include "bufstring.h"
#include "cubesampling.h"
#include "executor.h"
#include "emsurfaceauxdata.h"
#include "stattype.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class uiCalcStratAmp;

namespace EM { class Horizon3D; }

class CalcStratAmp  : public Executor
{
public:

    			CalcStratAmp( const IOObj*, const EM::Horizon3D*,
				      const EM::Horizon3D*, 
				      Stats::Type, const HorSampling& );
    			~CalcStratAmp();

    void                clear();
    int                 nextStep();
    int			totalNr() const;
    int			nrDone() const		{ return nrdone_; }
    const char*		message() const		{ return "Computing..."; }
    const char*		nrDoneText() const	{ return "Points done"; }    

    void		setOffsets(float top,float bot)
			{ tophorshift_ = top; bothorshift_ = bot; }
    int			init( const char* attribnm , bool addtotop );

protected:

    bool		usesinglehor_;
    Stats::Type		stattyp_;

    SeisTrcReader*      rdr_;
    BufferString        errmsg_;
    HorSampling         hs_;
    CubeSampling        cs_;

    const EM::Horizon3D*      tophorizon_;
    const EM::Horizon3D*      bothorizon_;

    int                 nrdone_;
    mutable int         totnr_;
    float               tophorshift_;
    float               bothorshift_;
    bool                createReader();
    void                stratalamp();
    EM::PosID           posid_;
    int                 dataidx_;
    bool		addtotop_;
};
#endif

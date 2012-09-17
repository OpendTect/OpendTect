#ifndef tutseistools_h
#define tutseistools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: tutseistools.h,v 1.1 2012/03/26 14:33:18 cvsdgb Exp $
-*/

#include "executor.h"
#include "cubesampling.h"
#include "samplingdata.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;


namespace Tut
{

mClass SeisTools : public Executor
{
public:

    enum Action		{ Scale, Square, Smooth, ChgSD };

    			SeisTools();
    virtual		~SeisTools();
    void		clear();

    const IOObj*	input() const		{ return inioobj_; }
    const IOObj*	output() const		{ return outioobj_; }
    inline Action	action() const		{ return action_; }
    inline float	factor() const		{ return factor_; }
    inline float	shift() const		{ return shift_; }
    inline SamplingData<float> sampling() const { return newsd_; }
    inline bool		weakSmoothing() const	{ return weaksmooth_; }

    void		setInput(const IOObj&);
    void		setOutput(const IOObj&);
    void		setRange(const CubeSampling&);
    inline void		setAction( Action a )	{ action_ = a; }
    inline void		setScale( float f, float s )
						{ factor_ = f; shift_ = s; }
    void		setSampling( SamplingData<float> sd )
    						{ newsd_ = sd; }
    inline void		setWeakSmoothing( bool yn )
    						{ weaksmooth_ = yn; }

			// Executor compliance functions
    const char*		message() const;
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    const char*		nrDoneText() const	{ return "Traces handled"; }
			// This is where it actually happens
    int			nextStep();

protected:

    IOObj*		inioobj_;
    IOObj*		outioobj_;
    CubeSampling	cs_;
    Action		action_;
    float		factor_;
    float		shift_;
    SamplingData<float>	newsd_;
    bool		weaksmooth_;

    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    SeisTrc&		trcin_;
    SeisTrc&            trcout_;
    int			nrdone_;
    mutable int		totnr_;
    BufferString	errmsg_;

    bool		createReader();
    bool		createWriter();
    void		handleTrace();

};

} // namespace

#endif

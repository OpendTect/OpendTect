#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "samplingdata.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;


namespace Tut
{

mExpClass(Tut) SeisTools : public Executor
{ mODTextTranslationClass(SeisTools);
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
    void		setRange(const TrcKeyZSampling&);
    inline void		setAction( Action a )	{ action_ = a; }
    inline void		setScale( float f, float s )
						{ factor_ = f; shift_ = s; }
    void		setSampling( SamplingData<float> sd )
    						{ newsd_ = sd; }
    inline void		setWeakSmoothing( bool yn )
    						{ weaksmooth_ = yn; }

			// Executor compliance functions
    uiString		uiMessage() const;
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    uiString		uiNrDoneText() const	{ return tr("Traces handled"); }
			// This is where it actually happens
    int			nextStep();

protected:

    IOObj*		inioobj_;
    IOObj*		outioobj_;
    TrcKeyZSampling	tkzs_;
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
    uiString		errmsg_;

    bool		createReader();
    bool		createWriter();
    void		handleTrace();

};

} // namespace

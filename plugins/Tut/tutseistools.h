#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tutmod.h"
#include "executor.h"
#include "samplingdata.h"
#include "trckeyzsampling.h"

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
    od_int64		totalNr() const override;
    od_int64		nrDone() const override { return nrdone_; }
    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override
					    { return tr("Traces handled"); }
			// This is where it actually happens

protected:

    IOObj*		inioobj_    = nullptr;
    IOObj*		outioobj_   = nullptr;
    TrcKeyZSampling	tkzs_;
    Action		action_;
    float		factor_     = 1.f;
    float		shift_	    = 0.f;
    SamplingData<float>	newsd_;
    bool		weaksmooth_ = false;

    SeisTrcReader*	rdr_	    = nullptr;
    SeisTrcWriter*	wrr_	    = nullptr;
    SeisTrc&		trcin_;
    SeisTrc&            trcout_;
    int			nrdone_     = 0;
    int			totnr_	    = -1;
    uiString		msg_;

    bool		createReader();
    bool		createWriter();
    void		handleTrace();

    bool		goImpl(od_ostream*,bool,bool,int) override;

private:
    int			nextStep() override;
    void		calculateTotalNr();

};

} // namespace

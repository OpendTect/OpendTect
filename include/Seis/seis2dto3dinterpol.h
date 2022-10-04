#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisbuf.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "binid.h"
#include "uistring.h"
#include "od_ostream.h"
#include "factory.h"

class IOObj;
class Seis2DDataSet;
class SeisScaler;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcBuf;
class od_ostream;
namespace Seis { class Provider; }

mExpClass(Seis) Seis2DTo3DInterPol : public Executor
{ mODTextTranslationClass(Seis2DTo3DInterPol)
public:
    mDefineFactoryInClass(Seis2DTo3DInterPol,factory);
			Seis2DTo3DInterPol();
    virtual		~Seis2DTo3DInterPol();

    const char*		message() const override
			{ return errmsg_ ? "interpolating"
						   : errmsg_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    const char*		nrDoneText() const override	{ return "Done"; }
    od_int64		totalNr() const override;
    int			nextStep() override;
    void		setStream(od_ostream&);
    void		setTaskRunner(TaskRunner* );
    virtual bool	init(const IOPar&);

    static const char*	sKeyInput();
    static const char*	sKeyType();
    static const char*	sKeyPow();
    static const char*	sKeyTaper();
    static const char*	sKeySmrtScale();
    static const char*	sKeyCreaterType();
    static BufferString getCreatorFormat() { return "Experimental"; }

protected:
    virtual bool		usePar(const IOPar&);
    virtual bool		setIO(const IOPar&);
    virtual bool		checkParameters();

    IOObj*			inioobj_;
    IOObj*			outioobj_;
    TrcKeyZSampling		tkzs_;
    const char*			errmsg_;

    SeisTrcBuf&			seisbuf_;
    TrcKeySampling		seisbuftks_;
    SeisTrcWriter*		wrr_;
    SeisTrcReader*		rdr_;
    SeisTrcBuf			tmpseisbuf_;

    bool			read_;
    int				nrdone_;
    mutable int			totnr_;
    bool			read();

    Array3D<float_complex>*	trcarr_;
    Array3D<float_complex>*	butterfly_;
    Array3D<float_complex>*	geom_;

    Fourier::CC*		fft_;
    bool			smartscaling_;
	float			rmsmax_;
    float			pow_;
    TaskRunner*			taskrun_;
    od_ostream*			strm_;

    float			taperangle_;
    bool			readData();
    bool			readInputCube(const int szfastx,
				    const int szfasty, const int szfastz );
    bool			butterflyOperator();
    bool			scaleArray();
    void			smartScale();
    bool			writeOutput();
    virtual bool		preProcessArray() {return true;}
    virtual bool		unProcessArray() {return true;}
};



mExpClass(Seis) Seis2DTo3DInterPolImpl : public Seis2DTo3DInterPol
{ mODTextTranslationClass(Seis2DTo3DInterPolImpl);
public:
	mDefaultFactoryInstantiation(
		Seis2DTo3DInterPol,
		Seis2DTo3DInterPolImpl,
		"Basic",
		tr("Basic"))

protected:
				Seis2DTo3DInterPolImpl();
				~Seis2DTo3DInterPolImpl();

};

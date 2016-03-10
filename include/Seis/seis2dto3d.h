#ifndef seis2dto3d_h
#define seis2dto3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id$
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

mExpClass(Seis) Seis2DTo3D : public Executor
{ mODTextTranslationClass(Seis2DTo3D)
public:
	mDefineFactoryInClass(Seis2DTo3D,factory);

			Seis2DTo3D();
			~Seis2DTo3D();

    uiString		uiMessage() const
			{ return errmsg_.isEmpty() ? tr("interpolating")
						   : errmsg_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		uiNrDoneText() const	{ return tr("Done"); }
    od_int64		totalNr() const;
    int				nextStep();
	void			setStream(od_ostream&);
	void			setTaskRunner(TaskRunner* );
    virtual bool	init(const IOPar&);

    static const char*	sKeyInput();
	static const char*	sKeyType();
    static const char*	sKeyPow();
    static const char*	sKeyTaper();
    static const char*	sKeySmrtScale();

protected:
    virtual bool		usePar(const IOPar&);
    virtual bool		setIO(const IOPar&);
    virtual bool		checkParameters();

    IOObj*			inioobj_;
    IOObj*			outioobj_;
    TrcKeyZSampling	tkzs_;
    uiString		errmsg_;

    SeisTrcBuf&		seisbuf_;
    TrcKeySampling	seisbuftks_;
    SeisTrcWriter*  wrr_;
    SeisTrcReader*	rdr_;
    SeisTrcBuf		tmpseisbuf_;

    bool			read_;
    int				nrdone_;
    mutable int		totnr_;
    bool			read();

    Array3D<float_complex>*		trcarr_;
    Array3D<float_complex>*		butterfly_;
    Array3D<float_complex>*		geom_;

    Fourier::CC*	fft_;
    bool			smartscaling_;
	float			rmsmax_;
    float			pow_;
    TaskRunner*		taskrun_;
	od_ostream*		strm_;

    float	taperangle_;
    bool	readData();
    bool	readInputCube(const int szfastx,
				    const int szfasty, const int szfastz );
    bool	butterflyOperator();
    bool			scaleArray();
    void			smartScale();
    bool			writeOutput();
	virtual bool	preProcessArray() {return true;}
	virtual bool	unProcessArray() {return true;}
};

mExpClass(Seis) Seis2DTo3DImpl : public Seis2DTo3D
{ mODTextTranslationClass(Seis2DTo3DImpl);
public:
	mDefaultFactoryInstantiation(
		Seis2DTo3D,
		Seis2DTo3DImpl,
		"Basic",
		tr("Basic"))

protected:

};

#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/

#include "seisbuf.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "binid.h"
#include "uistrings.h"
#include "od_ostream.h"
#include "factory.h"

class Seis2DDataSet;
class SeisScaler;
class SeisTrc;
class SeisTrcBuf;
class od_ostream;
namespace Seis { class Provider; class Storer; }

mExpClass(Seis) Seis2DTo3D : public Executor
{ mODTextTranslationClass(Seis2DTo3D)
public:
	mDefineFactoryInClass(Seis2DTo3D,factory);

			Seis2DTo3D();
			~Seis2DTo3D();

    uiString		message() const
			{ return errmsg_.isEmpty() ? uiStrings::sInterpolating()
						   : errmsg_; }
    od_int64		nrDone() const	   { return nrdone_; }
    uiString		nrDoneText() const { return tr("Number interpolated"); }
    od_int64		totalNr() const;
    int			nextStep();
    void		setStream(od_ostream&);
    void		setTaskRunner(TaskRunner* );
    virtual bool	init(const IOPar&);

    static const char*	sKeyInput();
    static const char*	sKeyType();
    static const char*	sKeyPow();
    static const char*	sKeyTaper();
    static const char*	sKeySmrtScale();

protected:

    virtual bool	usePar(const IOPar&);
    virtual bool	setIO(const IOPar&);
    virtual bool	checkParameters();

    IOObj*		inioobj_;
    IOObj*		outioobj_;
    TrcKeyZSampling	tkzs_;
    uiString		errmsg_;

    SeisTrcBuf&		seisbuf_;
    TrcKeySampling	seisbuftks_;
    Seis::Provider*	prov_;
    Seis::Storer*	storer_;
    SeisTrcBuf		tmpseisbuf_;

    bool		read_;
    int			nrdone_;
    mutable int		totnr_;
    bool		read();

    Array3D<float_complex>* trcarr_;
    Array3D<float_complex>* butterfly_;
    Array3D<float_complex>* geom_;

    Fourier::CC*	fft_;
    bool		smartscaling_;
    float		rmsmax_;
    float		pow_;
    TaskRunner*		taskrun_;
    od_ostream*		strm_;

    float		taperangle_;
    bool		readData();
    bool		readInputCube(const int szfastx,
				    const int szfasty, const int szfastz );
    bool		butterflyOperator();
    bool		scaleArray();
    void		smartScale();
    bool		writeOutput();
    virtual bool	preProcessArray()	{ return true; }
    virtual bool	unProcessArray()	{ return true; }
};


mExpClass(Seis) Seis2DTo3DImpl : public Seis2DTo3D
{
public:

	mDefaultFactoryInstantiation(
		Seis2DTo3D,
		Seis2DTo3DImpl,
		"Basic",
		uiStrings::sBasic())

};

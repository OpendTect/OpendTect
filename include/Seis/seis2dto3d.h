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


#include "seismod.h"
#include "seismod.h"
#include "executor.h"
#include "cubesampling.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "binid.h"
#include "seisbuf.h"

class IOObj;
class Seis2DDataSet;
class SeisScaler;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcBuf;


mExpClass(Seis) SeisInterpol : public Executor
{
public:

    			SeisInterpol();
			~SeisInterpol();

    void		setInput(const ObjectSet<const SeisTrc>&);
    void		setParams(const HorSampling&,float maxvel);

    void		getOutTrcs(ObjectSet<SeisTrc>&,
					const HorSampling&) const;
    const char*		errMsg() const 		{ return errmsg_.isEmpty() ? 0
						       : errmsg_.buf();  }
    od_int64           	nrDone() const 		{ return nrdone_; }
    uiString		uiNrDoneText() const	{return "Number of iterations";}
    od_int64		totalNr() const 	{ return nriter_; };
    int			nextStep();

protected:

    const ObjectSet<const SeisTrc>* inptrcs_; 
    int			nriter_;
    float 		maxvel_;
    BufferString	errmsg_;

    int			nrdone_;
    mutable int		totnr_;

    Fourier::CC*        fft_;
    int 		szx_;
    int 		szy_;
    int 		szz_;
    float 		max_;

    mStruct(Seis) TrcPosTrl
    {
		    TrcPosTrl(int x,int y, int trc)
			: idx_(x)
			, idy_(y)
			, trcpos_(trc)   
			{}     
	int idx_;
	int idy_;
	int trcpos_;

	bool operator	== ( const TrcPosTrl& tr ) const
			    { return tr.trcpos_ == trcpos_; }
    };
    TypeSet<TrcPosTrl> 		posidxs_;

    Array3DImpl<float_complex>*  trcarr_;
    HorSampling		hs_;

    void		clear();
    void		doPrepare();
    void		setUpData();
    void		setFinalTrcs();

    const BinID		convertToBID(int,int) const;
    void		convertToPos(const BinID&,int&,int&) const;
    int			getTrcInSet(const BinID&) const;
};



mExpClass(Seis) Seis2DTo3D : public Executor
{
public:

    			Seis2DTo3D();
			~Seis2DTo3D();

    void		setInput(const IOObj& lineset);
    void		setOutput(IOObj& cube,const CubeSampling& outcs);

    void		setParams(int inl,int crl,float maxvel,bool reuse);
    void		setIsNearestTrace( bool yn );

    const char*		errMsg() const 		{ return errmsg_.isEmpty() ? 0
						       : errmsg_.buf();  }

    uiString		uiMessage() const	{ return "interpolating"; }
    od_int64           	nrDone() const 		{ return nrdone_; }
    uiString		uiNrDoneText() const	{return "Done";}
    od_int64		totalNr() const;
    int			nextStep();

protected:

    const Seis2DDataSet* ds_;
    IOObj*		outioobj_;
    CubeSampling	cs_;
    BinID		prevbid_;
    int			nriter_;

    float 		maxvel_;
    bool		reusetrcs_;
    int			inlstep_;
    int			crlstep_;

    BufferString	errmsg_;

    SeisScaler*		sc_;

    SeisTrcBuf&		seisbuf_;
    SeisTrcWriter*      wrr_;
    SeisTrcBuf		tmpseisbuf_;

    SeisInterpol	interpol_;
    HorSamplingIterator hsit_;

    bool		read_;	
    int			nrdone_;
    mutable int		totnr_;
    bool		nearesttrace_;

    void		clear();
    bool		writeTmpTrcs();
    bool		read();
};


mExpClass(Seis) SeisScaler
{
public:
			SeisScaler(const SeisTrcBuf&);

    void                scaleTrace(SeisTrc&);
protected:

    float               avgmaxval_;
    float               avgminval_;
};

#endif



#ifndef seis2dto3d_h
#define seis2dto3d_h


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id: seis2dto3d.h,v 1.1 2011-02-21 14:18:30 cvsbruno Exp $
________________________________________________________________________

-*/


#include "executor.h"
#include "cubesampling.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "position.h"

class IOObj;
class Seis2DLineSet;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcBuf;

mClass Seis2DTo3D : public Executor
{
public:

    			Seis2DTo3D();
			~Seis2DTo3D();

    const CubeSampling&	setInput(const IOObj& lineset,const char* attrnm);
    void		setOutput(IOObj& cube,const CubeSampling* outcs);
    void		setNrIter(int);

    const char*		errMsg() const 		{ return errmsg_.isEmpty() ? 0
						       : errmsg_.buf();  }
    const char*		message() const		{ return "interpolating"; }
    od_int64           	nrDone() const 		{ return nrdone_; }
    od_int64		totalNr() const 	{ return nriter_; };
    const char*        	nrDoneText() const 	{return "Number of iterations";}
    int			nextStep();

protected:

    const Seis2DLineSet* ls_;
    IOObj*		outioobj_;
    CubeSampling	cs_;
    int			nriter_;
    BufferString	errmsg_;
    BufferString	attrnm_;

    SeisTrcBuf&		seisbuf_;
    SeisTrcWriter*      wrr_;

    int			nrdone_;
    mutable int		totnr_;

    Fourier::CC*        fft_;
    int 		szx_;
    int 		szy_;
    int 		szz_;
    float 		max_;

    mStruct TrcPosTrl
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

    ObjectSet< Array1DImpl<float_complex> > fftsignals1d_;
    ObjectSet< Array2DImpl<float_complex> > signals2d_;

    void		clear();
    bool		write();
    bool		read();
    void		doPrepare();
    void		setUpData();
    void		setFinalTrcs();

    const BinID		convertToBID(int,int) const;
    const SeisTrc*	getTrcInSet(const BinID&) const;
};


#endif

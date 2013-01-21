#ifndef fourier_h
#define fourier_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl/Y. Liu
 Date:		August 2001
 RCS:		$Id$
________________________________________________________________________

*/

#include "algomod.h"
#include "transform.h"

#include "factory.h"


namespace Fourier
{

class FFTCC1D;

/*!
\ingroup Algo
\brief Does Fourier Transforms of any size.
*/

mExpClass(Algo) CC : public GenericTransformND
{
public:
    mDefaultFactoryInstantiation( CC, CC, "PFAFFT", "FFT" );
    static ::Factory<CC>& factory();

    static CC*		createDefault();

    			CC();
    void		setNormalization(bool yn); 
    static float	getNyqvist(float samplespacing);
    static float	getDf(float samplespacing,int nrsamples);
    bool		isFast(int sz) const;
    virtual int		getFastSize(int sz) const;
			/*!<Returns a size that is equal or larger than sz */
    
    
    static void pfarc(int isign,int n,const float* rz,float_complex* cz);
    static void pfacr(int isign,int n,const float_complex*,float* rz);
    static int npfaro(int nmin, int nmax);
    static int npfao(int nmin, int nmax);

protected:
    static void pfacc(char dir,int sz,int step,float_complex* signal);
    /*!<Prime number size FFT where the signal has a sampling not equal to 1, 
      i.e. every Nth value should be used.
      \param dir 1 for forward, -1 for backwards
      \param sz Length of signal
      \param step is the step between the samples of the signal
      \param signal Is the signal itself. The output will be written to signal.
    */
    static void pfacc(char dir,int sz,int step,int nr,const int* starts,
		      float_complex* signal);
    static void pfacc(char dir,int sz,int step,int nr,int batchstep,
		      float_complex* signal);
    static void pfacc(char dir,int sz,float_complex*);

    bool		setup();
    bool		normalize_;

    Transform1D*	createTransform() const;
    class CC1D : public GenericTransformND::Transform1D, public ParallelTask
    {
	public:
			CC1D();
			~CC1D();
	bool            init();
	bool            run(bool parallel)	{ return execute( parallel ); }
	od_int64        nrIterations() const	{ return nr_; }
	bool            doPrepare(int);
	bool            doWork(od_int64 start, od_int64 stop, int );
	void		setNormalization(bool yn)	{ normalize_ = yn; }

	static int	getFastSize(int sz);
			/*!<Returns a size that is equal or larger than sz */

    protected:

	bool            	dopfa_;
	char			direction_;
	int			higheststart_;
	bool			normalize_;
	ObjectSet<FFTCC1D>	ffts_;
    };
};


/*!
\ingroup Algo
\brief Computes FFT for any size of data.
*/

mExpClass(Algo) FFTCC1D
{
public:
    			FFTCC1D();
			FFTCC1D(const FFTCC1D&);
			//!<Not implemented, just here to make linker complain

    			~FFTCC1D()			{ cleanUp(); }
    bool		setSize(int); 
    			//!<the size of fft to be calculated
    void		setSample(int smp)		{ sample_ = smp; }
    			//!<step of the data from input	
    void		setDir(bool forward)		{ forward_ = forward; }
    void		setNormalization(bool yn)	{ normalize_ = yn; } 
    bool		run(float_complex* data);

protected:

    void		cleanUp();
    bool		getSizeFactors();
    bool		doFactor2() const;
    			//!<ret true means the whole FFT is over
    bool		doFactor4() const;
    void		doFactor3() const;
    void		doFactor5() const;
    void		doOtherFactor(int factor,int psz);
    void		doRotation(int psz) const;
    bool		doFinish();
    bool		setupPermutation();

    bool		forward_;
    bool		normalize_;
    int			size_;
    int			sample_;
    int			totalsmp_;
    int			extsz_;
    int			curf_;
    int			rmfid_;
    int			cycleid_;
    float		exp_;
    float		sin2_;

    float_complex*	data_;
    float*		rdata_;
    float*		idata_;
    float*		rtmp_;
    float*		itmp_;
    float*		cosv_;
    float*		sinv_;
    int*		permutation0_;
    int*		permutation1_;
    TypeSet<int>	permutfactors_;    
    TypeSet<int>	factors_;
};


};//end of namespace Fourier


#endif


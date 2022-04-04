#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl/Y. Liu
 Date:		August 2001
________________________________________________________________________

*/

#include "algomod.h"
#include "transform.h"
#include "paralleltask.h"
#include "factory.h"


namespace Fourier
{

class FFTCC1D;

/*!
\brief Does Fourier Transforms of any size.
*/

mExpClass(Algo) CC : public GenericTransformND
{ mODTextTranslationClass(CC);
public:
    static ::Factory<CC>& factory();
    virtual uiString factoryDisplayName() const;
    virtual const char* factoryKeyword() const;
    mDefaultFactoryCreatorImpl(CC,CC);
    mDefaultStaticFactoryStringDeclaration;

    static CC*		createDefault();
    static void         initClass();

			CC();
    void		setNormalization(bool yn);
    static float	getNyqvist(float samplespacing);
    static float	getDf(float samplespacing,int nrsamples);
    static void		getFrequencies(float samplespacing,int nrsamples,
				       TypeSet<float>&);
    bool		isFast(int sz) const;
    virtual int		getFastSize(int sz) const;
			/*!<Returns a size that is equal or larger than sz */

    static void pfarc(int isign,int n,const float* rz,float_complex* cz);
    static void pfacr(int isign,int n,const float_complex*,float* rz);
    static int npfaro(int nmin, int nmax);
    static int npfao(int nmin, int nmax);

protected:
    static uiString* legalInfo();
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

    bool		setup() override;
    bool		normalize_ = false;

    Transform1D*	createTransform() const override;
    mClass(Algo) CC1D : public GenericTransformND::Transform1D,
			public ParallelTask
    {
	public:
			CC1D();
			~CC1D();
	bool            init() override;
	bool            run( bool parallel ) override
			{ return executeParallel( parallel ); }
	od_int64        nrIterations() const override	{ return nr_; }
	bool            doPrepare(int) override;
	bool            doWork(od_int64 start, od_int64 stop, int ) override;
	void		setNormalization(bool yn)	{ normalize_ = yn; }

	static int	getFastSize(int sz);
			/*!<Returns a size that is equal or larger than sz */

    protected:

	bool	dopfa_;
	char			direction_;
	int			higheststart_;
	bool			normalize_;
	ObjectSet<FFTCC1D>	ffts_;
    };
};


/*!
\brief Computes 1D FFT for any size of data. This function is used internally
       by the ND computation.
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

    static int		getFastSize(int sz);

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

} // namespace Fourier


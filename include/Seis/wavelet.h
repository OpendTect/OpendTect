#ifndef wavelet_h
#define wavelet_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "namedobj.h"
#include "ranges.h"
#include "transl.h"
#include "tableascio.h"
class Conn;
class IOObj;


mExpClass(Seis) Wavelet : public NamedObject
{
public:
			Wavelet(const char* nm=0);
			Wavelet(bool ricker_else_sinc,float fpeak,
				float sample_intv=mUdf(float),float scale=1);
			Wavelet(const Wavelet&);
    Wavelet&		operator=(const Wavelet&);
    virtual		~Wavelet();

    static Wavelet*	get(const IOObj*);
    bool		put(const IOObj*) const;

    int			size() const		{ return sz_; }
    float*		samples()		{ return samps_; }
    const float*	samples() const		{ return samps_; }
    float		sampleRate() const	{ return dpos_; }
    int			centerSample() const	{ return cidx_; }
    StepInterval<float>	samplePositions() const
    			{ return StepInterval<float>(
				-cidx_*dpos_, (sz_-cidx_-1)*dpos_, dpos_ ); }
    bool		hasSymmetricalSamples()	{ return cidx_ * 2 + 1 == sz_; }

    void		setSampleRate(float sr)	{ dpos_ = sr; }
    void		setCenterSample(int cidx)	{ cidx_ = cidx; }
    			//!< positive for starttwt < 0
    void		reSize(int); // destroys current sample data!

    bool		reSample(float newsr);
    bool		reSampleTime(float newsr);
    void		ensureSymmetricalSamples();
    			//!< pads with zeros - use with and before reSample
   			//  for better results
    void		transform(float,float);
    void		normalize();
    float		getExtrValue(bool ismax = true) const;

    static void		markScaled(const MultiID& id); //!< "External"
    static void		markScaled(const MultiID& id,const MultiID& orgid,
	    			   const MultiID& horid,const MultiID& seisid,
				   const char* lvlnm);
    static bool		isScaled(const MultiID&);
    static bool		isScaled(const MultiID& id,MultiID& orgid,
					MultiID& horid,MultiID& seisid,
					BufferString& lvlnm);
    					//!< if external, orgid will be "0"

protected:

    float		dpos_;
    float*		samps_;
    int			sz_;
    int			cidx_;		//!< The index of the center sample

};


mExpClass(Seis) WaveletTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(Wavelet)
public:
    			mDefEmptyTranslatorGroupConstructor(Wavelet)

    const char*		 defExtension() const		{ return "wvlt"; }
};

mExpClass(Seis) WaveletTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Wavelet)

    virtual bool	read(Wavelet*,Conn&)		= 0;
    virtual bool	write(const Wavelet*,Conn&)	= 0;

};



mExpClass(Seis) dgbWaveletTranslator : public WaveletTranslator
{			     isTranslator(dgb,Wavelet)
public:
    			mDefEmptyTranslatorConstructor(dgb,Wavelet)

    bool		read(Wavelet*,Conn&);
    bool		write(const Wavelet*,Conn&);

};


mExpClass(Seis) WaveletAscIO : public Table::AscIO
{
public:
    				WaveletAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    Wavelet*			get(std::istream&) const;
    bool			put(std::ostream&) const;

};


#endif


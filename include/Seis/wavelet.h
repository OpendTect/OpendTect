#ifndef wavelet_h
#define wavelet_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: wavelet.h,v 1.14 2006-11-06 15:07:44 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "namedobj.h"
#include "transl.h"
#include "ranges.h"
class Conn;
class IOObj;


class Wavelet : public NamedObject
{
public:
			Wavelet(const char* nm=0,int idxfsamp=0,
				float sr=mUdf(float));
			Wavelet(bool ricker_else_sinc,float fpeak,
				float sample_intv=mUdf(float),float scale=1);
			Wavelet(const Wavelet&);
    Wavelet&		operator=(const Wavelet&);
    virtual		~Wavelet();

    static Wavelet*	get(const IOObj*);
    bool		put(const IOObj*) const;

    int			size() const		{ return sz; }
    float*		samples()		{ return samps; }
    const float*	samples() const		{ return samps; }
    float		sampleRate() const	{ return dpos; }
    int			centerSample() const	{ return -iw; }
    StepInterval<float>	samplePositions() const
    			{ return StepInterval<float>( iw*dpos, (sz+iw-1)*dpos,
						      dpos ); }

    void		reSize(int); // destroys current sample data!
    void		set(int center,float samplerate);

    void		transform(float,float);

protected:

    int			iw;		// The index of the first sample
					// where the center is 0
    float		dpos;
    float*		samps;
    int			sz;

};


class WaveletTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(Wavelet)
public:
    			mDefEmptyTranslatorGroupConstructor(Wavelet)

    const char*		 defExtension() const		{ return "wvlt"; }
};

class WaveletTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Wavelet)

    virtual bool	read(Wavelet*,Conn&)		= 0;
    virtual bool	write(const Wavelet*,Conn&)	= 0;

};



class dgbWaveletTranslator : public WaveletTranslator
{			     isTranslator(dgb,Wavelet)
public:
    			mDefEmptyTranslatorConstructor(dgb,Wavelet)

    bool		read(Wavelet*,Conn&);
    bool		write(const Wavelet*,Conn&);

};


namespace Table { class FormatDesc; }

class WaveletAscIO
{
public:
    				WaveletAscIO( const Table::FormatDesc& fd )
				    : fd_(fd)			{}

    static Table::FormatDesc*	getDesc();
    const Table::FormatDesc&	desc() const		{ return fd_; }

    Wavelet*			get(std::istream&) const;
    bool			put(std::ostream&) const;

    const char*			errMsg() const		{ return errmsg_; }

protected:

    mutable BufferString	errmsg_;
    const Table::FormatDesc&	fd_;

};


#endif

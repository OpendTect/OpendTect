#ifndef wavelet_H
#define wavelet_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: wavelet.h,v 1.6 2003-10-15 15:15:53 bert Exp $
________________________________________________________________________

-*/
 
#include "uidobj.h"
#include "transl.h"
class Conn;
class IOObj;


class Wavelet : public UserIDObject
{
public:
			Wavelet(const char* nm=0,int idxfsamp=0,float sr=0.004);
			Wavelet(const Wavelet&);
    Wavelet&		operator=(const Wavelet&);
    virtual		~Wavelet();

    static Wavelet*	get(const IOObj*);
    int			put(const IOObj*) const;

    int			size() const		{ return sz; }
    float*		samples()		{ return samps; }
    const float*	samples() const		{ return samps; }
    float		sampleRate() const	{ return dpos; }
    unsigned short	suDt() const	{ return (unsigned short)(dpos*1e6+.5); }
    int			centerSample() const	{ return -iw; }

    void		reSize(int); // destroys info present!
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

    virtual int		read(Wavelet*,Conn&)		= 0;
    virtual int		write(const Wavelet*,Conn&)	= 0;

};



class dgbWaveletTranslator : public WaveletTranslator
{			     isTranslator(dgb,Wavelet)
public:
    			mDefEmptyTranslatorConstructor(dgb,Wavelet)

    int			read(Wavelet*,Conn&);
    int			write(const Wavelet*,Conn&);

};


#endif

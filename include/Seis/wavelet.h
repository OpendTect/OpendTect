#ifndef wavelet_H
#define wavelet_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: wavelet.h,v 1.3 2001-10-18 09:37:13 windev Exp $
________________________________________________________________________

@$*/
 
#include <defobj.h>
#include <uidobj.h>
class IOObj;

class Wavelet : public DefObject
	      , public UserIDObject
{		isUidConcreteDefObject(Wavelet)
public:
			Wavelet(const char* nm=0,int idxfsamp=0,float sr=0.004);
			Wavelet(istream&);
			Wavelet(const Wavelet&);
    Wavelet&		operator=(const Wavelet&);
    int			write(ostream&) const;
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


#include <transl.h>
#include <ctxtioobj.h>
class Conn;


class WaveletTranslator : public Translator
{			  isTranslatorGroup(Wavelet)
public:
			WaveletTranslator( const char* nm = 0 )
			: Translator(nm)		{}
    virtual		~WaveletTranslator()		{}

    virtual int		read(Wavelet*,Conn&)		{ return NO; }
    virtual int		write(const Wavelet*,Conn&)	{ return NO; }

    static int		selector(const char*);
    static const IOObjContext&	ioContext();

};



class dgbWaveletTranslator : public WaveletTranslator
{			     isTranslator(dgb,Wavelet)
public:

    int			read(Wavelet*,Conn&);
    int			write(const Wavelet*,Conn&);

};


#endif

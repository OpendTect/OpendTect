#ifndef waveletio_h
#define waveletio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "transl.h"
#include "tableascio.h"
class Wavelet;


mExpClass(Seis) WaveletTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(Wavelet);
    mODTextTranslationClass(WaveletTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(Wavelet)

    const char*		 defExtension() const		{ return "wvlt"; }
};

mExpClass(Seis) WaveletTranslator : public Translator
{ mODTextTranslationClass(WaveletTranslator);
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
{ mODTextTranslationClass(WaveletAscIO);
public:
				WaveletAscIO( const Table::FormatDesc& fd )
				    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    Wavelet*			get(od_istream&) const;
    bool			put(od_ostream&) const;

};


#endif

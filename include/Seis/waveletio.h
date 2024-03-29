#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "transl.h"
#include "tableascio.h"
class Wavelet;


mExpClass(Seis) WaveletTranslatorGroup : public TranslatorGroup
{					 isTranslatorGroup(Wavelet)
public:
			mDefEmptyTranslatorGroupConstructor(Wavelet)

    const char*		 defExtension() const override	{ return "wvlt"; }
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
			~dgbWaveletTranslator();

    bool		read(Wavelet*,Conn&) override;
    bool		write(const Wavelet*,Conn&) override;

};


mExpClass(Seis) WaveletAscIO : public Table::AscIO
{ mODTextTranslationClass(WaveletAscIO);
public:
				WaveletAscIO( const Table::FormatDesc& fd );
				~WaveletAscIO();

    static Table::FormatDesc*	getDesc();

    Wavelet*			get(od_istream&) const;
    bool			put(od_ostream&) const;

};

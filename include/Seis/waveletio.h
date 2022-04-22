#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2015
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

    bool		read(Wavelet*,Conn&) override;
    bool		write(const Wavelet*,Conn&) override;

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



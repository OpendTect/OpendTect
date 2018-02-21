#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "saveable.h"
#include "wavelet.h"
#include "transl.h"
#include "tableascio.h"
#include "ptrman.h"


/*!\brief Loader for Wavelet. Loads into WaveletMGR(). */

mExpClass(Seis) WaveletLoader
{ mODTextTranslationClass(WaveletLoader)
public:

			WaveletLoader(const DBKey&);
			WaveletLoader(const IOObj*);
			~WaveletLoader();

    uiRetVal		load();

protected:

    IOObj*		ioobj_;

public:

    uiRetVal		read(Wavelet*&);
    bool		addToMGR(Wavelet*,const DBKey&);

};


/*!\brief Saveable for Wavelet. */

mExpClass(Seis) WaveletSaver : public Saveable
{ mODTextTranslationClass(WaveletSaver)
public:

			WaveletSaver(const Wavelet&);
			mDeclMonitorableAssignment(WaveletSaver);
			~WaveletSaver();

    ConstRefMan<Wavelet> wavelet() const;
    void		setWavelet(const Wavelet&);

    mDeclInstanceCreatedNotifierAccess(WaveletSaver);

protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;

};



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

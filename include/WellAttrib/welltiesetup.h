#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "enums.h"
#include "wellio.h"

class SynthGenParams;
class od_istream;
class od_ostream;


#define mIsUnvalidD2TM(wd) ( !wd->haveD2TModel() || wd->d2TModel()->size()<2 )

namespace WellTie
{

mExpClass(WellAttrib) Setup
{ mODTextTranslationClass(Setup)
public:
			enum CorrType { None, Automatic, UserDefined };
			mDeclareEnumUtils(CorrType)

				Setup();
				Setup(const Setup&);
				~Setup();

    Setup&			operator =(const Setup&);

    MultiID			wellid_;
    MultiID			seisid_;
    SynthGenParams&		sgp_;
    BufferString		linenm_;	// Empty: 3D
    BufferString		seisnm_;
    BufferString		denlognm_;
    BufferString		vellognm_;
    BufferString		svellognm_;
    bool			issonic_ = true;
    bool			isshearsonic_ = true;
    bool			useexistingd2tm_ = true;
    CorrType			corrtype_ = Automatic;

    void			supportOldPar(const IOPar&);
    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    static Setup&		defaults();
    static void			commitDefaults();

    static const char*		sKeyCSCorrType();
    static const char*		sKeyUseExistingD2T();
    static const uiString	sCSCorrType();
    static const uiString	sUseExistingD2T();
};


mExpClass(WellAttrib) IO : public Well::odIO
{
public:
				IO(const char* fnm,uiString& errmsg);
				~IO();

    static const char*		sKeyWellTieSetup();

};


mExpClass(WellAttrib) Writer : public IO
{
public:
				Writer(const char* fnm);
				~Writer();

    bool			putWellTieSetup(const WellTie::Setup&) const;

    bool			putIOPar(const IOPar&,const char*) const;

protected:

    uiString			errmsg_;

    bool			putIOPar(const IOPar&,const char*,
					 od_ostream&) const;
    bool			wrHdr(od_ostream&,const char*) const;

};


mExpClass(WellAttrib) Reader : public IO
{
public:
				Reader(const char* fnm);
				~Reader();

    void			getWellTieSetup(WellTie::Setup&) const;

    IOPar*			getIOPar(const char*) const;

protected:

    uiString			errmsg_;

    IOPar*			getIOPar(const char*,od_istream&) const;

};

} // namespace WellTie

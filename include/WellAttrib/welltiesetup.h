#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "namedobj.h"

#include "enums.h"
#include "multiid.h"
#include "wellio.h"
#include "od_iosfwd.h"


#define mIsUnvalidD2TM(wd) ( !wd->haveD2TModel() || wd->d2TModel()->size()<2 )

namespace WellTie
{

mExpClass(WellAttrib) Setup
{ mODTextTranslationClass(Setup)
public:
			enum CorrType { None, Automatic, UserDefined };
			mDeclareEnumUtils(CorrType)

			Setup()
			    : linenm_(*new BufferString) //empty = data is 3D
			    , issonic_(true)
			    , useexistingd2tm_(true)
			    , corrtype_(Automatic)
			    {}


				Setup( const Setup& setup )
				    : wellid_(setup.wellid_)
				    , seisid_(setup.seisid_)
				    , wvltid_(setup.wvltid_)
				    , linenm_(setup.linenm_)
				    , issonic_(setup.issonic_)
				    , seisnm_(setup.seisnm_)
				    , vellognm_(setup.vellognm_)
				    , denlognm_(setup.denlognm_)
				    , useexistingd2tm_(setup.useexistingd2tm_)
				    , corrtype_(setup.corrtype_)
				    {}

    MultiID			wellid_;
    MultiID			seisid_;
    MultiID			wvltid_;
    BufferString		linenm_;
    BufferString		seisnm_;
    BufferString		vellognm_;
    BufferString		denlognm_;
    bool			issonic_;
    bool			useexistingd2tm_;
    CorrType			corrtype_;

    void			supportOldPar(const IOPar&);
    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    static Setup&		defaults();
    static void			commitDefaults();

    static const char*		sKeyCSCorrType()
				{ return "CheckShot Corrections"; }
    static const char*		sKeyUseExistingD2T()
				{ return "Use Existing Depth/Time model"; }
    static const uiString	sCSCorrType()
				{ return tr("CheckShot Corrections"); }
    static const uiString	sUseExistingD2T()
				{ return tr("Use Existing Depth/Time model"); }
};


mExpClass(WellAttrib) IO : public Well::odIO
{
public:
				IO( const char* f, uiString& errmsg )
				: Well::odIO(f,errmsg)	{}

    static const char*		sKeyWellTieSetup();

};


mExpClass(WellAttrib) Writer : public IO
{
public:
				Writer( const char* f )
				    : IO(f,errmsg_) {}

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
				Reader( const char* f )
				    : IO(f,errmsg_) {}

    void			getWellTieSetup(WellTie::Setup&) const;

    IOPar*			getIOPar(const char*) const;

protected:

    uiString			errmsg_;

    IOPar*			getIOPar(const char*,od_istream&) const;

};

} // namespace WellTie


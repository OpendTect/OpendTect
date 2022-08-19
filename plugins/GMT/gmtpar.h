#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtmod.h"

#include "gmtdef.h"
#include "iopar.h"
#include "manobjectset.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "uistring.h"


mExpClass(GMT) GMTPar : public IOPar
{
public:
			GMTPar( const IOPar& par, const char* workdir )
			    : IOPar(par),workingdir_(workdir)	{}

    bool		execute(od_ostream&,const char*);

    virtual const char* userRef() const					= 0;
    virtual bool	fillLegendPar(IOPar&) const	{ return false; }

    bool		execCmd(const OS::MachineCommand&,od_ostream& logstrm,
				const char* fnm=nullptr,bool append=true);
    od_ostream		makeOStream(const OS::MachineCommand&,
				    od_ostream& logstrm,
				    const char* fnm=nullptr,bool append=true);

    static OS::MachineCommand	getWrappedComm(const OS::MachineCommand&);
    static BufferString getErrFnm();

   const char*		getWorkDir() const	{ return workingdir_.buf(); }

private:

    virtual bool	doExecute(od_ostream&,const char*)		= 0;
    static void		checkErrStrm(const char*,od_ostream&);

    BufferString	workingdir_;

};


typedef GMTPar* (*GMTParCreateFunc)(const IOPar&,const char*);

mExpClass(GMT) GMTParFactory
{
public:

    int			add(const char* nm, GMTParCreateFunc);
    GMTPar*		create(const IOPar&,const char* workdir) const;

    const char*		name(int) const;
    int			size() const	{ return entries_.size(); }

protected:

    mStruct(GMT) Entry
    {
				Entry(	const char* nm,
					GMTParCreateFunc fn )
				    : name_(nm)
				    , crfn_(fn)		{}

	BufferString		name_;
	GMTParCreateFunc	crfn_;
    };

    ManagedObjectSet<Entry>	entries_;

    Entry*			getEntry(const char*) const;

    friend mGlobal(GMT) GMTParFactory&	GMTPF();
};

mGlobal(GMT) GMTParFactory& GMTPF();


#define mErrStrmRet(s) { strm << s << '\n'; return false; }

#define mGetRangeString( str ) \
    Interval<float> xrg, yrg, mapdim; \
    get( ODGMT::sKeyXRange(), xrg ); \
    get( ODGMT::sKeyYRange(), yrg ); \
    str = "-R"; str += xrg.start; str += "/"; \
    str += xrg.stop; str += "/"; \
    str += yrg.start; str += "/"; str += yrg.stop;

#define mGetProjString( str, projkey ) \
    Interval<float> dim; \
    get( ODGMT::sKeyMapDim(), dim ); \
    str += "-J"; str += projkey; str += dim.start; str += "c/"; \
    str += dim.stop; str += "c";

#define mGetColorString( col, str ) \
    str = (int) col.r(); \
    str += "/"; str += (int) col.g(); \
    str += "/"; str += (int) col.b();

#define mGetLineStyleString( ls, str ) \
    str = ls.width_; str += "p,"; \
    BufferString lscol; \
    mGetColorString( ls.color_, lscol ); \
    str += lscol; str += ","; \
    switch ( ls.type_ ) \
    { \
	case OD::LineStyle::Dash: \
	    str += "-"; \
	    break; \
	case OD::LineStyle::Dot: \
	    str += "."; \
	    break; \
	case OD::LineStyle::DashDot: \
	    str += "-."; \
	    break; \
	case OD::LineStyle::DashDotDot: \
	    str += "-.."; \
	    break; \
	default: break; \
    }

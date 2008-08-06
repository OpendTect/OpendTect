#ifndef gmtpar_h
#define gmtpar_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtpar.h,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/

#include "iopar.h"
#include "gmtdef.h"


class GMTPar : public IOPar
{
public:
    			GMTPar(const char* nm)
			    : IOPar(nm)	{}
			GMTPar(const IOPar& par)
			    : IOPar(par) {}

    virtual bool	execute(std::ostream&,const char*)		=0;
    virtual const char* userRef() const					=0;
    virtual bool	fillLegendPar(IOPar&) const	{ return false; }
};


typedef GMTPar* (*GMTParCreateFunc)(const IOPar&);

class GMTParFactory
{
public:

    int			add(const char* nm, GMTParCreateFunc);
    GMTPar*		create(const IOPar&) const;

    const char*		name(int) const;
    int			size() const	{ return entries_.size(); }

protected:

    struct Entry
    {
				Entry(	const char* nm,
					GMTParCreateFunc fn )
				    : name_(nm)
				    , crfn_(fn)		{}

	BufferString		name_;
	GMTParCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*) const;

    friend GMTParFactory&	GMTPF();
};

GMTParFactory& GMTPF();


#define mErrStrmRet(s) { strm << s << std::endl; return false; }

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
	case LineStyle::Dash: \
	    str += "-"; \
	    break; \
	case LineStyle::Dot: \
	    str += "."; \
	    break; \
	case LineStyle::DashDot: \
	    str += "-."; \
	    break; \
	case LineStyle::DashDotDot: \
	    str += "-.."; \
	    break; \
	default: \
	    break; \
    }

#endif

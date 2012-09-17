#ifndef gmtbasemap_h
#define gmtbasemap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtbasemap.h,v 1.4 2009/07/22 16:01:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"


class GMTBaseMap : public GMTPar
{
public:

    static void		initClass();

    			GMTBaseMap(const char* nm)
			    : GMTPar(nm)	{}
			GMTBaseMap(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const			{ return 0; }

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


class GMTLegend : public GMTPar
{
public:

    static void		initClass();

			GMTLegend(const char* nm)
			    : GMTPar(nm)	{}
			GMTLegend(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const			{ return 0; }

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};


class GMTCommand : public GMTPar
{
public:

    static void		initClass();

    			GMTCommand(const char* nm)
			    : GMTPar(nm)	{}
			GMTCommand(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMTBaseMap : public GMTPar
{
public:

    static void		initClass();

			GMTBaseMap( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override		{ return 0; }

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


mClass(GMT) GMTLegend : public GMTPar
{
public:

    static void		initClass();

			GMTLegend( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir)	{}

    const char*		userRef() const override		{ return 0; }

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


mClass(GMT) GMTCommand : public GMTPar
{
public:

    static void		initClass();

			GMTCommand( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


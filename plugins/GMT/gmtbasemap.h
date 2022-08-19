#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

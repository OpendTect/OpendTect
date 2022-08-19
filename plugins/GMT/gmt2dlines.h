#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtpar.h"


mClass(GMT) GMT2DLines : public GMTPar
{
public:

    static void		initClass();

			GMT2DLines( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

    static void		postText(const Coord&,int fontsz,float angle,
				 const char* justify,const char* txt,
				 bool modern,od_ostream&,int gmt4fontno=4);

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};


mClass(GMT) GMTRandLines : public GMTPar
{
public:

    static void		initClass();

			GMTRandLines( const IOPar& par, const char* workdir )
			    : GMTPar(par,workdir) {}

    const char*		userRef() const override;
    bool		fillLegendPar(IOPar&) const override;

protected:

    virtual bool	doExecute(od_ostream&,const char*) override;

    static GMTPar*	createInstance(const IOPar&,const char*);
    static int		factoryid_;
};

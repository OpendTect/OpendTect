#ifndef gmtclip_h
#define gmtclip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
 RCS:           $Id: gmtclip.h,v 1.1 2011-05-10 03:51:28 cvsraman Exp $
________________________________________________________________________

-*/


#include "gmtpar.h"


class GMTClip : public GMTPar
{
public:

    static void		initClass();

    			GMTClip(const char* nm)
			    : GMTPar(nm)	{}
			GMTClip(const IOPar& par)
			    : GMTPar(par) {}

    virtual bool	execute(std::ostream&,const char*);
    virtual const char* userRef() const;
    bool		isStart() const;
    bool		fillLegendPar(IOPar&) const;

protected:

    static GMTPar*	createInstance(const IOPar&);
    static int		factoryid_;
};

#endif

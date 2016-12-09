#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "geometrymod.h"
#include "uistring.h"
#include "probe.h"

namespace Geometry { class RandomLine; }

mExpClass(Geometry) RandomLineProbe : public Probe
{
public:
				RandomLineProbe( int rdmlineid =-1);
    static Geometry::RandomLine* createNewDefaultRDL();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static const char*		sRandomLineID();
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    mImplSimpleMonitoredGet(randomeLineID,int,rdlid_)
    void			setRandomLineID(int rdlid);
    void			geomUpdated();
    mDeclInstanceCreatedNotifierAccess( RandomLineProbe );
    mDeclAbstractMonitorableAssignment( RandomLineProbe );

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		createName() const;
    int				rdlid_;
};

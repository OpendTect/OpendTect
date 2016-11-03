#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "geometrymod.h"
#include "probe.h"


mExpClass(Geometry) RDLProbe : public Probe
{
public:
				RDLProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static const char*		sRandomLineID();
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    mImplSimpleMonitoredGet(randomeLineID,int,rdlid_)
    void			setRDLID(int rdlid);
    mDeclInstanceCreatedNotifierAccess( RDLProbe );
    mDeclAbstractMonitorableAssignment( RDLProbe );
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		createName() const;
    int				rdlid_;
};

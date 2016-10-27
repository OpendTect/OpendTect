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


mExpClass(General) RDLProbe : public Probe
{
public:
				RDLProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static const char*		sRandomLineID();
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    mImplSimpleMonitoredGetSet(inline,randomeLineID,setRDLID,int,rdlid_,
			       cPositionChange())
    mDeclInstanceCreatedNotifierAccess( RDLProbe );
    mDeclAbstractMonitorableAssignment( RDLProbe );
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		createName() const;
    int				rdlid_;
};

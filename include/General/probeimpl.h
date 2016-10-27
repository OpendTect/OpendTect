#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "generalmod.h"
#include "probe.h"
#include "survgeom.h"

mExpClass(General) InlineProbe : public Probe
{
public:
				InlineProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    mDeclInstanceCreatedNotifierAccess( InlineProbe );
    mDeclAbstractMonitorableAssignment( InlineProbe );

protected:
    BufferString		createName() const;
};


mExpClass(General) CrosslineProbe : public Probe
{
public:
				CrosslineProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    mDeclInstanceCreatedNotifierAccess( CrosslineProbe );
    mDeclAbstractMonitorableAssignment( CrosslineProbe );

protected:
    BufferString		createName() const;
};


mExpClass(General) ZSliceProbe : public Probe
{
public:
				ZSliceProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    mDeclInstanceCreatedNotifierAccess( ZSliceProbe );
    mDeclAbstractMonitorableAssignment( ZSliceProbe );

protected:
    BufferString		createName() const;
};


mExpClass(General) Line2DProbe : public Probe
{
public:
				Line2DProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    mImplSimpleMonitoredGetSet(inline,geomID,setGeomID,Pos::GeomID,geomid_,
			       cPositionChange())
    mDeclInstanceCreatedNotifierAccess( Line2DProbe );
    mDeclAbstractMonitorableAssignment( Line2DProbe );
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:
    BufferString		createName() const;
    Pos::GeomID			geomid_;
};

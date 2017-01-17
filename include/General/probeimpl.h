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

namespace ZDomain { class Info; }

mExpClass(General) InlineProbe : public Probe
{
public:
				InlineProbe(const TrcKeyZSampling&);
				InlineProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);
    bool			is3DSlice() const	{ return true; }

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    mDeclInstanceCreatedNotifierAccess( InlineProbe );
    mDeclAbstractMonitorableAssignment( InlineProbe );

    BufferString		getDisplayName() const;
};


mExpClass(General) CrosslineProbe : public Probe
{
public:
				CrosslineProbe(const TrcKeyZSampling&);
				CrosslineProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    mDeclInstanceCreatedNotifierAccess( CrosslineProbe );
    mDeclAbstractMonitorableAssignment( CrosslineProbe );
    bool			is3DSlice() const	{ return true; }

    BufferString		getDisplayName() const;
};


mExpClass(General) ZSliceProbe : public Probe
{
public:
				ZSliceProbe(const TrcKeyZSampling&);
				ZSliceProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    virtual bool		isVertical() const	{ return false; }
    mDeclInstanceCreatedNotifierAccess( ZSliceProbe );
    mDeclAbstractMonitorableAssignment( ZSliceProbe );
    bool			is3DSlice() const	{ return true; }

    BufferString		getDisplayName() const;
};


mExpClass(General) Line2DProbe : public Probe
{
public:
				Line2DProbe(Pos::GeomID);
				Line2DProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    virtual bool		is2D() const		{ return true; }
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    mImplSimpleMonitoredGet(geomID,Pos::GeomID,geomid_);
    mDeclInstanceCreatedNotifierAccess( Line2DProbe );
    mDeclAbstractMonitorableAssignment( Line2DProbe );
    void			setGeomID(Pos::GeomID);
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    BufferString		getDisplayName() const;

protected:
    Pos::GeomID			geomid_;
};


mExpClass(General) VolumeProbe : public Probe
{
public:
				VolumeProbe(const TrcKeyZSampling&);
				VolumeProbe();
    static const char*		sFactoryKey();
    virtual const char*		type() const		{ return sFactoryKey();}
    static Probe*		createFrom(const IOPar&);

    static void			initClass();
    virtual bool		usePar(const IOPar&);
    void			setZDomain(const ZDomain::Info&);
    mDeclInstanceCreatedNotifierAccess( VolumeProbe );
    mDeclAbstractMonitorableAssignment( VolumeProbe );
    BufferString		getDisplayName() const;

protected:
    ZDomain::Info*		zdomain_;
};

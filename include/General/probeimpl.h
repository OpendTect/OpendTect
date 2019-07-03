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
#include "uistrings.h"

namespace ZDomain { class Info; }

mExpClass(General) InlineProbe : public Probe
{
public:
			InlineProbe(const TrcKeyZSampling&);
			InlineProbe();
			~InlineProbe();
			mDeclInstanceCreatedNotifierAccess(InlineProbe);
			mDeclMonitorableAssignment( InlineProbe );

			mDeclRequiredProbeFns();

    virtual bool	is3DSlice() const override	{ return true; }

    virtual bool	usePar(const IOPar&) override;

};


mExpClass(General) CrosslineProbe : public Probe
{
public:
			CrosslineProbe(const TrcKeyZSampling&);
			CrosslineProbe();
			~CrosslineProbe();
			mDeclInstanceCreatedNotifierAccess(CrosslineProbe);
			mDeclMonitorableAssignment(CrosslineProbe);

			mDeclRequiredProbeFns();

    virtual bool	is3DSlice() const override	{ return true; }

    virtual bool	usePar(const IOPar&) override;

};


mExpClass(General) ZSliceProbe : public Probe
{
public:

			ZSliceProbe(const TrcKeyZSampling&);
			ZSliceProbe();
			~ZSliceProbe();
			mDeclInstanceCreatedNotifierAccess(ZSliceProbe);
			mDeclMonitorableAssignment(ZSliceProbe);

			mDeclRequiredProbeFns();

    virtual bool	is3DSlice() const	{ return true; }
    virtual bool	isVertical() const	{ return false; }

    virtual bool	usePar(const IOPar&);

};


mExpClass(General) Line2DProbe : public Probe
{
public:

    mUseType( Pos,	GeomID );

			Line2DProbe(GeomID);
			Line2DProbe();
			~Line2DProbe();
			mDeclInstanceCreatedNotifierAccess(Line2DProbe);
			mDeclMonitorableAssignment(Line2DProbe);

			mDeclRequiredProbeFns();

    bool		is2D() const override		{ return true; }

			mImplSimpleMonitoredGet(geomID,GeomID,geomid_);
    void		setGeomID(GeomID);
    virtual void	fillPar(IOPar&) const override;
    virtual bool	usePar(const IOPar&) override;

protected:

    GeomID		geomid_;

};


mExpClass(General) VolumeProbe : public Probe
{
public:
			VolumeProbe(const TrcKeyZSampling&);
			VolumeProbe();
			~VolumeProbe();
			mDeclInstanceCreatedNotifierAccess(VolumeProbe);
			mDeclMonitorableAssignment(VolumeProbe);

			mDeclRequiredProbeFns();

    virtual bool	usePar(const IOPar&) override;
    void		setZDomain(const ZDomain::Info&);

protected:

    ZDomain::Info*	zdominfo_;

};

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2014
 RCS:		$Id: emregion.h 38690 2015-03-30 18:00:30Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/


#include "emcommon.h"
#include "namedobj.h"

#include "dbkey.h"
#include "polygon.h"
#include "position.h"
#include "taskrunner.h"
#include "trckeyzsampling.h"

class FaultTrcDataProvider;

namespace EM
{

class Fault;
class Horizon3D;

mExpClass(EarthModel) Region
{
public:
    virtual			~Region();

    int				id() const;
    const TrcKeyZSampling&	getBoundingBox() const;
    virtual bool		isInside(const TrcKey&,float z,
					 bool includeborder=true) const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

				Region(Pos::GeomID gid=Pos::GeomID::get3D());

    TrcKeyZSampling		tkzs_;
    int				id_;
    Pos::GeomID			geomid_;

};


mExpClass(EarthModel) RegionBoundary : public NamedObject
{
public:
    virtual const char*	type() const				= 0;
    virtual bool	init(const TaskRunnerProvider&)		{ return true; }
    virtual bool	hasName() const				{ return false;}

    virtual bool	onRightSide(const TrcKey&,float z) const = 0;
    virtual void	getSideStrs(uiStringSet&) const		= 0;
    virtual void	setSide( int sd )			{ side_ = sd; }
    int			getSide() const				{ return side_;}

    virtual void	fillPar(IOPar&) const			= 0;
    virtual bool	usePar(const IOPar&)			= 0;

protected:
			RegionBoundary()
			    : side_(-1)	{}

    int			side_;
};


mExpClass(EarthModel) RegionInlBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionInlBoundary)
public:
			RegionInlBoundary( int inl=mUdf(int) )
			    : inl_(inl)
			{}

    const char*		type() const;
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    int			inl_;
};


mExpClass(EarthModel) RegionCrlBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionCrlBoundary)
public:
			RegionCrlBoundary( int crl=mUdf(int) )
			    : crl_(crl)
			{}

    const char*		type() const;
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    int			crl_;
};


mExpClass(EarthModel) RegionZBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionZBoundary)
public:
			RegionZBoundary( float z=mUdf(float) )
			    : z_(z) {}

    const char*		type() const;
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    float		z_;
};


mExpClass(EarthModel) RegionHor3DBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionHor3DBoundary)
public:
			RegionHor3DBoundary(const DBKey&);
			~RegionHor3DBoundary();

    const char*		type() const;
    void		setKey(const DBKey&);
    DBKey		key() const		{ return key_; }
    bool		hasName() const		{ return true; }
    bool		init(const TaskRunnerProvider&);
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    DBKey		key_;
    EM::Horizon3D*	hor_;

};


mExpClass(EarthModel) RegionFaultBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionFaultBoundary)
public:
			RegionFaultBoundary(const DBKey&);
			~RegionFaultBoundary();

    const char*		type() const;
    void		setKey(const DBKey&);
    DBKey		key() const		{ return key_; }
    bool		hasName() const		{ return true; }
    bool		init(const TaskRunnerProvider&);
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    DBKey			key_;
    const EM::Fault*		flt_;
    FaultTrcDataProvider&	prov_;

};


mExpClass(EarthModel) RegionPolygonBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionPolygonBoundary)
public:
			RegionPolygonBoundary(const DBKey&);
			~RegionPolygonBoundary();

    const char*		type() const;
    void		setKey(const DBKey&);
    DBKey		key() const		{ return key_; }
    bool		hasName() const		{ return true; }
    void		getSideStrs(uiStringSet&) const;
    bool		onRightSide(const TrcKey&,float z) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    DBKey			key_;
    const ODPolygon<float>*	polygon_;
};


mExpClass(EarthModel) Region3D : public Region
{
public:
			Region3D();
			~Region3D();

    void		addBoundary(RegionBoundary*); // becomes mine
    RegionBoundary*	getBoundary(int idx);
    void		removeBoundary(int idx);
    void		removeBoundary(RegionBoundary&);
    bool		hasBoundary(const DBKey&) const;
    int			size() const;
    bool		isEmpty() const;
    void		setEmpty();

    bool		init(const TaskRunnerProvider&);
    bool		isInside(const TrcKey&,float z,bool inclborder) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    ObjectSet<RegionBoundary>	boundaries_;
};

} // namespace EM

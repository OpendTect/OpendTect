#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "namedobj.h"

#include "multiid.h"
#include "polygon.h"
#include "position.h"
#include "trckeyzsampling.h"

class FaultTrcDataProvider;
class TaskRunner;

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
				Region(Pos::GeomID);

    TrcKeyZSampling		tkzs_;
    int				id_;
    Pos::GeomID			geomid_;

};


mExpClass(EarthModel) RegionBoundary : public NamedObject
{
public:
			~RegionBoundary();

    virtual const char*	type() const				= 0;
    virtual bool	init(TaskRunner*)			{ return true; }
    virtual bool	hasName() const				{ return false;}

    virtual bool	onRightSide(const TrcKey&,float z) const = 0;
    virtual void	getSideStrs(uiStringSet&) const		= 0;
    virtual void	setSide( int sd )			{ side_ = sd; }
    int			getSide() const				{ return side_;}

    virtual void	fillPar(IOPar&) const			= 0;
    virtual bool	usePar(const IOPar&)			= 0;

protected:
			RegionBoundary();

    int			side_		= -1;
};


mExpClass(EarthModel) RegionInlBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionInlBoundary)
public:
			RegionInlBoundary(int inl=mUdf(int));
			~RegionInlBoundary();

    const char*		type() const override;
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    int			inl_;
};


mExpClass(EarthModel) RegionCrlBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionCrlBoundary)
public:
			RegionCrlBoundary(int crl=mUdf(int));
			~RegionCrlBoundary();

    const char*		type() const override;
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    int			crl_;
};


mExpClass(EarthModel) RegionZBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionZBoundary)
public:
			RegionZBoundary(float z=mUdf(float));
			~RegionZBoundary();

    const char*		type() const override;
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    float		z_;
};


mExpClass(EarthModel) RegionHor3DBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionHor3DBoundary)
public:
			RegionHor3DBoundary(const MultiID&);
			~RegionHor3DBoundary();

    const char*		type() const override;
    void		setKey(const MultiID&);
    const MultiID&	key() const			{ return key_; }
    bool		hasName() const override	{ return true; }
    bool		init(TaskRunner*) override;
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    MultiID		key_;
    EM::Horizon3D*	hor_;
};


mExpClass(EarthModel) RegionFaultBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionFaultBoundary)
public:
			RegionFaultBoundary(const MultiID&);
			~RegionFaultBoundary();

    const char*		type() const override;
    void		setKey(const MultiID&);
    const MultiID&	key() const			{ return key_; }
    bool		hasName() const override	{ return true; }
    bool		init(TaskRunner*) override;
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    MultiID			key_;
    const EM::Fault*		flt_;
    FaultTrcDataProvider&	prov_;

};


mExpClass(EarthModel) RegionPolygonBoundary : public RegionBoundary
{ mODTextTranslationClass(RegionPolygonBoundary)
public:
			RegionPolygonBoundary(const MultiID&);
			~RegionPolygonBoundary();

    const char*		type() const override;
    void		setKey(const MultiID&);
    const MultiID&	key() const			{ return key_; }
    bool		hasName() const override	{ return true; }
    void		getSideStrs(uiStringSet&) const override;
    bool		onRightSide(const TrcKey&,float z) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    MultiID			key_;
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
    bool		hasBoundary(const MultiID&) const;
    int			size() const;
    bool		isEmpty() const;
    void		setEmpty();

    bool		init(TaskRunner*);
    bool		isInside(const TrcKey&,float z,
				 bool inclborder) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    ObjectSet<RegionBoundary>	boundaries_;
};

} // namespace EM

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2012
________________________________________________________________________


-*/

#include "wellmod.h"
#include "multiid.h"
#include "posprovider.h"
#include "callback.h"
#include "trckeysampling.h"
#include "welldata.h"


namespace Pos
{

/*!
\brief Volume/Area provider based on Wells.
*/

mExpClass(Well) WellProvider3D : public Provider3D
			       , public CallBacker
{
public:
			WellProvider3D();
			WellProvider3D(const WellProvider3D&);
			~WellProvider3D();

    WellProvider3D&	operator=(const WellProvider3D&);
    const char* type() const override;	//!< sKey::Well()
    const char* factoryKeyword() const override { return type(); }
    Provider*	clone() const override	{ return new WellProvider3D(*this); }

    bool	initialize(TaskRunner* tr=0) override;
    void	reset() override		{ initialize(); }

    bool	toNextPos() override;
    bool	toNextZ() override;

    BinID	curBinID() const override	{ return curbid_; }
    float	curZ() const override		{ return curz_; }
    bool	includes(const BinID&,float z) const override;
    bool	includes(const Coord&,float z) const override;
    void	getSummary(BufferString&) const override;

    void	getExtent(BinID&,BinID&) const override;
    void	getZRange(Interval<float>&) const override;
    od_int64	estNrPos() const override;
    int		estNrZPerPos() const override	{ return zrg_.nrSteps()+1; }

    ConstRefMan<Well::Data>	wellData(int idx) const;
    StepInterval<float>&	zRange()		{ return zrg_; }
    const StepInterval<float>&	zRange() const		{ return zrg_; }
    TrcKeySampling&		horSampling()		{ return hs_; }
    const TrcKeySampling&	horSampling() const	{ return hs_; }

    void	usePar(const IOPar&) override;
    void	fillPar(IOPar&) const override;

    static const char*	sKeyInlExt();
    static const char*	sKeyCrlExt();
    static const char*	sKeyZExt();
    static const char*	sKeySurfaceCoords();
    static const char*	sKeyNrWells();

protected:

    void		setHS();
    TypeSet<MultiID>	wellids_;
    RefObjectSet<Well::Data> welldata_;

    bool		onlysurfacecoords_;
    int			inlext_;
    int			crlext_;
    float		zext_;
    TrcKeySampling&	hs_;
    TrcKeySamplingIterator hsitr_;
    StepInterval<float>	zrg_;

    BinID		curbid_;
    float		curz_;
    int			curwellidx_;
    bool		toNextWell();

public:

    static void		initClass();
    static Provider3D*	create()		{ return new WellProvider3D; }
};

} // namespace Pos


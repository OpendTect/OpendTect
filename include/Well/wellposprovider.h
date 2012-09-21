#ifndef wellposprovider_h
#define wellposprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2012
 RCS:           $Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "multiid.h"
#include "posprovider.h"

class HorSampling;
namespace Well { class Data; }


namespace Pos
{

/*!\brief Volume/Area provider based on Wells */

mClass(Well) WellProvider3D : public Provider3D
{
public:
			WellProvider3D();
			WellProvider3D(const WellProvider3D&);
			~WellProvider3D();

    WellProvider3D&	operator=(const WellProvider3D&);
    const char*		type() const;	//!< sKey::Well()
    const char*		factoryKeyword() const { return type(); }
    Provider*		clone() const	{ return new WellProvider3D(*this); }

    virtual bool	initialize(TaskRunner* tr=0);
    virtual void	reset()		{ initialize(); }

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	includes(const BinID&,float z) const;
    virtual bool	includes(const Coord&,float z) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID&,BinID&) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const	{ return zrg_.nrSteps()+1; }

    const Well::Data*	wellData(int idx) const;
    StepInterval<float>& zRange()		{ return zrg_; }
    const StepInterval<float>& zRange() const	{ return zrg_; }
    HorSampling&	horSampling()		{ return hs_; }
    const HorSampling&	horSampling() const	{ return hs_; }

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    static const char*	sKeyInlExt();
    static const char*	sKeyCrlExt();
    static const char*	sKeyZExt();
    static const char*	sKeySurfaceCoords();
    static const char*	sKeyNrWells();

protected:

    void		setHS();
    TypeSet<MultiID>	wellids_;
    ObjectSet<Well::Data> welldata_;

    bool		onlysurfacecoords_;
    int			inlext_;
    int			crlext_;
    float		zext_;
    HorSampling&	hs_;
    StepInterval<float>	zrg_;

    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new WellProvider3D; }
};

} // namespace Pos

#endif


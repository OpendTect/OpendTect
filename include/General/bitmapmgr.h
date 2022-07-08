#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "generalmod.h"

#include "array2dbitmap.h"
#include "datapack.h"
#include "geometry.h"

namespace FlatView { class Appearance; }
class FlatDataPack;

/*!
\brief Manages bitmaps
*/

mExpClass(General) BitMapMgr
{
public:
			BitMapMgr();
			~BitMapMgr();

    void		init(const FlatDataPack*,const FlatView::Appearance&,
			     bool wva);
    void		clearAll();

    Geom::Point2D<int>	dataOffs(const Geom::PosRectangle<double>&,
				 const Geom::Size2D<int>&) const;
			//!< Returns mUdf(int)'s when outside or incompatible

    bool		generate(const Geom::PosRectangle<double>& wr,
				 const Geom::Size2D<int>& bufwrsz,
				 const Geom::Size2D<int>& availpixels);
			//!< fails only when isufficient memory
    const A2DBitMap*	bitMap() const			{ return bmp_; }
    const A2DBitMapGenerator*	bitMapGen() const	{ return gen_; }

private:

    void				setup();

    mutable Threads::Lock		lock_;
    ConstRefMan<FlatDataPack>		datapack_;
    FlatView::Appearance&		appearance_;
    A2DBitMap*				bmp_ = nullptr;
    A2DBitMapPosSetup*			pos_ = nullptr;
    A2DBitMapInpData*			data_ = nullptr;
    A2DBitMapGenerator*			gen_ = nullptr;
    bool				wva_;

    Geom::Size2D<int>			sz_;
    Geom::PosRectangle<double>		wr_;
public:
    A2DBitMapGenerator*			bitMapGen()	{ return gen_; }
};


/*!
\brief Bitmap generation Task.
*/

mExpClass(General) BitMapGenTask : public Task
{
public:
		BitMapGenTask(BitMapMgr& mgr,
			const Geom::PosRectangle<double>& wr,
			const Geom::Size2D<int>& bufwrsz,
			const Geom::Size2D<int>& pix )
		    : mgr_(mgr), wr_(wr), bufwrsz_(bufwrsz), availpixels_(pix){}

    bool	execute() override
		{ return mgr_.generate(wr_,bufwrsz_,availpixels_); }

protected:

    BitMapMgr&				mgr_;
    const Geom::PosRectangle<double>&	wr_;
    const Geom::Size2D<int>&		bufwrsz_;
    const Geom::Size2D<int>&		availpixels_;
};


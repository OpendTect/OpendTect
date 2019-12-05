#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2019
________________________________________________________________________

*/

#include "seisblocks.h"
#include "binid.h"
#include "bin2d.h"

class ArrayNDInfo;
template <class T> class Array2D;
template <class T> class Array3D;


namespace Seis
{

class Storer;


namespace Blocks
{

class LineBuf;


/*!\brief Takes in 'small' tiles or blocks of (seismic) data and makes
  sure they are merged and written to storage. Hard requirement is that the data you feed is sorted on inl/crl or GeomID/trcnr. */

mExpClass(Seis) DataGlueer
{
public:

    mUseType( Seis,		Storer );
    mUseType( IdxPair,		pos_type );
    typedef float		val_type;
    typedef float		z_type;
    typedef int			idx_type;
    typedef Array2D<val_type>	Arr2D;
    typedef Array3D<val_type>	Arr3D;


		DataGlueer(Storer&);
		~DataGlueer();

    void	setSteps( pos_type stp, z_type zs )
		{ trcstep_ = stp; zstep_ = zs; }
    void	setSteps( const BinID& ps, z_type zs )
		{ trcstep_ = ps.crl(); linestep_ = ps.crl(); zstep_ = zs; }

    uiRetVal	addData(const Bin2D&,z_type midz,const Arr2D&);
    uiRetVal	addData(const BinID&,z_type midz,const Arr3D&);

    bool	is2D() const;

    uiRetVal	finish(); //!< Has to be called; check return value!

protected:

    Storer&		storer_;
    const ArrayNDInfo*	arrinfo_;
    pos_type		trcstep_	    = 1;
    pos_type		linestep_	    = 1;
    z_type		zstep_;
    Bin2D		curb2d_		    = Bin2D::udf();
    BinID		curbid_		    = BinID::udf();
    ObjectSet<LineBuf>	linebufs_;

    void	initGeometry(const ArrayNDInfo&);
    void	addPos(const Bin2D&,const Arr2D&,z_type);
    void	addPos(const BinID&,const Arr3D&,z_type);
    void	mergeZ(const Arr2D&,z_type);
    void	mergeZ(const Arr3D&,z_type);
    uiRetVal	storeFinished();

};


} // namespace Blocks

} // namespace Seis

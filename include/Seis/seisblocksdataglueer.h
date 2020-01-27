#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2019
________________________________________________________________________

*/

#include "binid.h"
#include "bin2d.h"
#include "seisblocks.h"
#include "seisbuf.h"

class ArrayNDInfo;
template <class T> class Array1D;
template <class T> class Array2D;
template <class T> class Array3D;
class SeisTrc;


namespace Seis
{

class SelData;
class Storer;


namespace Blocks
{

class LineBuf;


/*!\brief Takes in 'small' tiles or blocks of (seismic) data and makes
  sure they are merged and written to storage.

  Hard requirement is that the data you feed is sorted on inl/crl or
  GeomID/trcnr.

  The BinID/Bin2D and Z you provide denote the START of the tile/blocklet,
  not the center!

*/


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


		DataGlueer(const SelData&,Storer&);
		//!< SelData provides the enveloppe of possible samples
		~DataGlueer();

    void	setSteps( pos_type stp, z_type zs )
		{ trcstep_ = stp; zstep_ = zs; }
    void	setSteps( const BinID& ps, z_type zs )
		{ trcstep_ = ps.crl(); linestep_ = ps.inl(); zstep_ = zs; }

    uiRetVal	addData(const Bin2D&,z_type,const Arr2D&);
    uiRetVal	addData(const BinID&,z_type,const Arr3D&);

    bool	is2D() const;

    uiRetVal	finish(); //!< Has to be called; check return value!

protected:

    const SelData&	seissel_;
    Storer&		storer_;
    const ArrayNDInfo*	arrinfo_	    = nullptr;
    pos_type		linestep_	    = 1;
    pos_type		trcstep_	    = 1;
    z_type		zstep_;
    int			trcsz_		    = -1;
    Bin2D		curb2d_		    = Bin2D::udf();
    BinID		curbid_		    = BinID::udf();
    ObjectSet<LineBuf>	linebufs_;
    SeisTrcBuf		trcbuf_;

    void	initGeometry(const ArrayNDInfo&);
    void	addPos(const Bin2D&,const Arr2D&,z_type);
    void	addPos(const BinID&,const Arr3D&,z_type);
    uiRetVal	storeReadyPositions();

    LineBuf*	getBuf(pos_type);
    uiRetVal	storeLineBuf(const LineBuf&);
    void	fillTrace(SeisTrc&,Array1D<int>&);

};


} // namespace Blocks

} // namespace Seis

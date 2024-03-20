#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "enums.h"
#include "point3d.h"
#include "trckey.h"
#include "trckeyzsampling.h"

class ArrayNDInfo;
template <class T> class Array1D;
template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class ArrayND;
class SeisTrc;
class SeisTrcWriter ;

namespace PosInfo
{
    class CubeData;
}

namespace Seis
{

class LineBuf;
class SelData;
class TraceValues;

/*!\brief Takes in 'small' tiles or blocks of (seismic) data and makes
  sure they are merged and written to storage.

  Hard requirement is that the data you feed is sorted on inl/crl or
  GeomID/trcnr.

  The TrcKey and Z you provide denote the START of the tile/blocklet,
  not the center!

*/


mExpClass(Seis) DataGlueer
{
public:
		enum MergeMode { Average, Crop, Blend };
		mDeclareEnumUtils(MergeMode)

		DataGlueer(const TrcKeyZSampling&,SeisTrcWriter&,
			   Geom::Point3D<float> overlap, MergeMode merge=Blend);
		//!< SelData provides the envelope of possible samples
		~DataGlueer();

    void	setSteps( int stp, float zs )
		{ trcstep_ = stp; zstep_ = zs; }
    void	setSteps( const TrcKey& ps, float zs )
		{ trcstep_ = ps.crl(); linestep_ = ps.inl(); zstep_ = zs; }

    uiRetVal	addData(const TrcKey&,float,const Array2D<float>&);
    uiRetVal	addData(const TrcKey&,float,const Array3D<float>&);

    void	setTracePositions(const PosInfo::CubeData*);

    bool	is2D() const;

    uiRetVal	finish(); //!< Has to be called; check return value!

protected:

    TrcKeyZSampling	tkzs_;
    SeisTrcWriter&	storer_;
    const ArrayNDInfo*	arrinfo_	    = nullptr;
    int			linestep_	    = 1;
    int			trcstep_	    = 1;
    float		zstep_;
    int			trcsz_		    = -1;
    TrcKey		curtrcky_	    = TrcKey::udf();
    int			lastwrittenline_    = -1;
    ObjectSet<LineBuf>	linebufs_;
    PosInfo::CubeData*	lcd_		    = nullptr;
    MergeMode		mergemode_;
    ArrayND<float>*	weights_	    = nullptr;
    Geom::Point3D<float>	overlap_;

    void	initGeometry(const ArrayNDInfo&);
    void	addPos(const TrcKey&,const Array2D<float>&,float);
    void	addPos(const TrcKey&,const Array3D<float>&,float);
    uiRetVal	storeReadyPositions(bool force=false);

    LineBuf*	getBuf(int);
    uiRetVal	storeLineBuf(const LineBuf&);
    void	fillTrace(SeisTrc&,TraceValues&);

    StepInterval<int>	trcNrRange(int lnr) const;

};

} // namespace Seis

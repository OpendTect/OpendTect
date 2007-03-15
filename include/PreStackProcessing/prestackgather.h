#ifndef prestackgather_h
#define gather_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackgather.h,v 1.1 2007-03-15 17:28:52 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "multiid.h"
#include "position.h"
#include "refcount.h"
#include "samplingdata.h"

namespace PreStack
{

class Gather
{ mRefCountImpl(Gather);
public:
    				Gather();

    bool			readFrom( const MultiID&, const BinID&,
	    				  BufferString* errmsg=0 );

    const Array2DImpl<float>&	data() const		{ return data_; }
    Array2DImpl<float>&		data()			{ return data_; }
    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    const TypeSet<float>&	offsets() const		{ return offsets_; }
    const TypeSet<float>&	azimuths() const	{ return azimuths_; }
    const TypeSet<int>&		traceOrder() const	{ return traceorder_; }

    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isCorrected() const	{ return iscorr_; }
    bool			zIsTime() const		{ return zit_; }
    const SamplingData<float>&	zSampling() const	{ return zsampling_; }

    const BinID&		getBinID() const 	{ return binid_; }
    const MultiID&		getVelocityID() const	{ return velocitymid_; }

    static bool			getVelocityID(const MultiID& stor, MultiID& vid );

    static const char*		sKeyIsAngeGather();
    static const char*		sKeyIsNMO();
    static const char*		sKeyVelocityCubeID();
    static const char*		sKeyZisTime();

protected:

    Array2DImpl<float>		data_;

    MultiID			velocitymid_;
    bool			offsetisangle_;
    bool			iscorr_;
    TypeSet<float>		azimuths_;
    TypeSet<float>		offsets_;
    TypeSet<int>		traceorder_;
    SamplingData<float>		zsampling_;

    bool			zit_;
    BinID			binid_;
};

}; //namespace

#endif

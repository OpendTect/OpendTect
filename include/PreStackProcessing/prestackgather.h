#ifndef prestackgather_h
#define prestackgather_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackgather.h,v 1.2 2007-05-09 16:45:03 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "multiid.h"
#include "position.h"
#include "samplingdata.h"
#include "datapackbase.h"

namespace PreStack
{

class Gather : public FlatDataPack
{
public:
    				Gather();
    				~Gather();

    bool			readFrom(const MultiID&, const BinID&,
	    				 BufferString* errmsg=0);

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    const TypeSet<float>&	offsets() const		{ return offsets_; }
    const TypeSet<float>&	azimuths() const	{ return azimuths_; }
    const TypeSet<int>&		traceOrder() const	{ return traceorder_; }

    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isCorrected() const	{ return iscorr_; }
    bool			zIsTime() const		{ return zit_; }
    const StepInterval<double>&	zSampling() const;

    const BinID&		getBinID() const 	{ return binid_; }
    const MultiID&		getVelocityID() const	{ return velocitymid_; }

    static bool			getVelocityID(const MultiID& stor,MultiID& vid);

    static const char*		sKeyIsAngleGather();
    static const char*		sKeyIsNMO();
    static const char*		sKeyVelocityCubeID();
    static const char*		sKeyZisTime();

protected:

    MultiID			velocitymid_;
    bool			offsetisangle_;
    bool			iscorr_;
    TypeSet<float>		azimuths_;
    TypeSet<float>		offsets_;
    TypeSet<int>		traceorder_;

    bool			zit_;
    BinID			binid_;
};

}; //namespace

#endif

#ifndef prestackgather_h
#define prestackgather_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackgather.h,v 1.6 2007-10-03 14:01:33 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "multiid.h"
#include "position.h"
#include "samplingdata.h"
#include "seisbufadapters.h"

namespace PreStack
{

class Gather : public SeisTrcBufDataPack
{
public:
    				Gather();
    				Gather(const Gather&);
    				~Gather();

    bool			setSize( int nroff, int nrz );

    bool			readFrom(const MultiID&, const BinID&,
	    				 BufferString* errmsg=0);
    bool			isLoaded() const	{ return arr2d_; }

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;

    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isCorrected() const	{ return iscorr_; }
    bool			zIsTime() const		{ return zit_; }

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

    bool			zit_;
    BinID			binid_;
};

}; //namespace

#endif

#ifndef prestackgather_h
#define prestackgather_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: prestackgather.h,v 1.14 2008-12-15 22:47:07 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "multiid.h"
#include "position.h"
#include "offsetazimuth.h"
#include "samplingdata.h"
#include "datapackbase.h"

class IOObj;
class SeisPSReader;
class SeisTrcBufDataPack;

namespace PreStack
{

class Gather : public FlatDataPack
{
public:
    				Gather();
    				Gather(const Gather&);
    				~Gather();

    bool			is3D() const { return linename_.isEmpty(); }				
    bool			setSize( int nroff, int nrz );

    bool			readFrom(const MultiID&, const BinID&,
	    				 BufferString* errmsg=0);
    bool			readFrom(const IOObj&, const BinID&,
	    				 BufferString* errmsg=0);
    bool			readFrom(const IOObj&,SeisPSReader& rdr,
	    				 const BinID&,BufferString* errmsg=0);

    				//for 3d only
    const BinID&		getBinID() const 	{ return binid_; }

				//for 2D only.
    bool			readFrom(const IOObj&, const int tracenr, 
	    				 const char* linename,
    					 BufferString* errmsg=0);
    int				getSeis2DTraceNr() const { return binid_.crl; }
    const char*			getSeis2DName() const;

    bool			isLoaded() const	{ return arr2d_; }

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isCorrected() const	{ return iscorr_; }
    bool			zIsTime() const		{ return zit_; }


    const MultiID&		getVelocityID() const	{ return velocitymid_; }
    const MultiID&		getStorageID() const    { return storagemid_; }

    static bool			getVelocityID(const MultiID& stor,MultiID& vid);

    static const char*		sDataPackCategory();
    static const char*		sKeyIsAngleGather();
    static const char*		sKeyIsCorr();
    static const char*		sKeyZisTime();

    static const char*		sKeyPostStackDataID();
    static const char*		sKeyVelocityCubeID();
protected:

    MultiID			velocitymid_;
    MultiID			storagemid_;
    bool			offsetisangle_;
    bool			iscorr_;

    bool			zit_;
    BinID			binid_;
    TypeSet<float>		azimuths_;

    BufferString		linename_;
};

}; //namespace

#endif

#ifndef timedepthconv_h
#define timedepthconv_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: timedepthconv.h,v 1.12 2009-08-10 22:10:15 cvskris Exp $
________________________________________________________________________

*/


#include "zaxistransform.h"

#include "cubesampling.h"
#include "datapack.h"
#include "multidimstorage.h"
#include "veldesc.h"

class CubeDataPack;
class SeisTrc;
class FlatDataPack;
class MultiID;
class SeisTrcReader;
template <class T> class ValueSeries;

template <class T> class Array3D;


mClass VelocityStretcher : public ZAxisTransform
{
public:
    virtual bool		setVelData(const MultiID&)		= 0;
    const char*			errMsg() const 		{ return errmsg_; }

protected:
    				VelocityStretcher() : errmsg_( 0 ) 	{}
    mutable const char*		errmsg_;
};


/*!ZAxisstretcher that converts from time to depth (or back) using a
   velocity model on disk. */

mClass Time2DepthStretcher : public VelocityStretcher
{
public:
    static const char*		sName();
    const char*			name() const	{ return sName(); }
    static void			initClass();
    static ZAxisTransform*	create();

			Time2DepthStretcher();
    bool		setVelData(const MultiID&);
    bool		isOK() const;

    bool		needsVolumeOfInterest() const	{ return true; }
    int			addVolumeOfInterest(const CubeSampling&,bool);
    void		setVolumeOfInterest(int,const CubeSampling&,bool);
    void		removeVolumeOfInterest(int);
    bool		loadDataIfMissing(int,TaskRunner* =0);
    void		transform(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    const char*		getZDomainID() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelData() { return "Velocity volume"; }

protected:
    friend		class TimeDepthDataLoader;
			~Time2DepthStretcher();
    void		releaseData();
    Interval<float>	getTimeInterval(const BinID&,int voiidx) const;
    Interval<float>	getDepthInterval(const BinID&,int voiidx) const;

    static void				udfFill(ValueSeries<float>&,int);

    ObjectSet<Array3D<float> >		voidata_;
    TypeSet<CubeSampling>		voivols_;
    BoolTypeSet				voiintime_;
    TypeSet<int>			voiids_;

    SeisTrcReader*			velreader_;
    VelocityDesc			veldesc_;
    bool				velintime_;
};


/*!ZAxisstretcher that converts from depth to time (or back). Uses
   an Time2Depth converter to do the job. */


mClass Depth2TimeStretcher : public VelocityStretcher
{
public:
    static const char*		sName();
    const char*			name() const	{ return sName(); }
    static void			initClass();
    static ZAxisTransform*	create();

			Depth2TimeStretcher();
    bool		setVelData(const MultiID&);
    bool		isOK() const;

    bool		needsVolumeOfInterest() const;
    int			addVolumeOfInterest(const CubeSampling&,bool);
    void		setVolumeOfInterest(int,const CubeSampling&,bool);
    void		removeVolumeOfInterest(int);
    bool		loadDataIfMissing(int,TaskRunner* =0);
    void		transform(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    Interval<float>	getZInterval(bool from) const;
    float		getGoodZStep() const;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    const char*		getZDomainID() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
			~Depth2TimeStretcher()				{}

    RefMan<Time2DepthStretcher>		stretcher_;
};

#endif

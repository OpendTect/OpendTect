#ifndef timedepthsconv_h
#define timedepthsconv_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: timedepthconv.h,v 1.1 2007-09-21 20:47:48 cvskris Exp $
________________________________________________________________________

*/


#include "datapack.h"
#include "veldesc.h"
#include "zaxistransform.h"

class CubeDataPack;
class FlatDataPack;
class MultiID;
class SeisTrcReader;

template <class T> class Array3D;

class Time2DepthStretcher : public ZAxisTransform
{
public:
    static const char*		sName();
    const char*			name() const	{ return sName(); }
    static void			initClass();
    static ZAxisTransform*	create();

			Time2DepthStretcher();
    bool		setVelData(const MultiID&);
    bool		setVelData(DataPack::ID,const VelocityDesc&,
				   const char* depthkey=0);
    bool		isOK() const;

    bool		needsVolumeOfInterest() const	{ return true; }
    int			addVolumeOfInterest(const CubeSampling&,bool);
    //void		setVolumeOfInterest(int,const CubeSampling&,bool);
    //void		removeVolumeOfInterest(int);
    //bool		loadDataIfMissing(int);
    void		transform(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    void		transformBack(const BinID&,const SamplingData<float>&,
	    			  int,float*) const;
    Interval<float>	getZInterval(bool from) const;

    //void		fillPar(IOPar&) const;
    //bool		usePar(const IOPar&);

protected:
				~Time2DepthStretcher();
    void			releaseData();

    ObjectSet<Array3D<float> >	voidata_;
    TypeSet<CubeSampling>	voivols_;
    BoolTypeSet			voidepth_;
    TypeSet<int>		voiids_;

    FlatDataPack*		flatdp_;
    CubeDataPack*		cubedp_;

    SeisTrcReader*		velreader_;
    VelocityDesc		veldesc_;
    bool			velintime_;
};

#endif

#ifndef timedepthsconv_h
#define timedepthsconv_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: timedepthconv.h,v 1.4 2008-02-13 12:25:58 cvsnanne Exp $
________________________________________________________________________

*/


#include "datapack.h"
#include "veldesc.h"
#include "zaxistransform.h"
#include "multidimstorage.h"

class CubeDataPack;
class SeisTrc;
class FlatDataPack;
class MultiID;
class SeisTrcReader;
template <class T> class ValueSeries;

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

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelData() { return "Velocity volume"; }

protected:
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

#endif

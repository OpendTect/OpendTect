#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.31 2007-11-23 09:09:44 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"
#include "seistype.h"

class BinID;
class BinIDValueSet;
class LineKey;
class MultiID;
class SeisSelData;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcBuf;
class SeisTrcWriter;
class Scaler;

namespace Attrib
{
class DataHolder;
class DataCubes;
class Data2DHolder;

class Output
{ mRefCountImpl(Output);
public:
    				Output();

    virtual bool		getDesiredVolume(CubeSampling&) const
    				{ return true; }
    virtual bool		wantsOutput(const BinID&) const		 = 0;
    virtual const DataCubes*	getDataCubes() const 	{ return 0; }
    virtual DataCubes*		getDataCubes(float)	{ return 0; }

    virtual void		getDesiredOutputs( TypeSet<int>& outputs ) const
				{ outputs = desoutputs_; }
    void			setDesiredOutputs( const TypeSet<int>& outputs )
    				{ desoutputs_ = outputs; }

    virtual TypeSet<Interval<int> > getLocalZRanges(const BinID&,float,
	    					    TypeSet<float>&) const =0;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&)		 = 0;
    virtual void		writeTrc()		{};
    const SeisSelData&		getSelData()		{ return seldata_; }
    const LineKey&		curLineKey() const;
    virtual void		adjustInlCrlStep(const CubeSampling&)	{};

    static const char*		outputstr;
    static const char*          cubekey;
    static const char*          tskey;
    static const char*          surfkey;
    static const char*          scalekey;
    static const char*          varzlinekey;

protected:
    SeisSelData&		seldata_;
    TypeSet<int>		desoutputs_;
};


class DataCubesOutput : public Output
{
public:
				DataCubesOutput(const CubeSampling&);
				~DataCubesOutput();

    const DataCubes*		getDataCubes() const;
    virtual DataCubes*		getDataCubes(float);

    bool			getDesiredVolume(CubeSampling&) const;
    void			setGeometry(const CubeSampling&);
    void                	setUndefValue( float v )	{ udfval_ = v; }

    bool			wantsOutput(const BinID&) const;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    					TypeSet<float>&) const;
    virtual void		adjustInlCrlStep(const CubeSampling&);
    
protected:
    CubeSampling		desiredvolume_;
    TypeSet< Interval<int> >	sampleinterval_;
    DataCubes*			datacubes_;
    float			udfval_;

    void			init(float);
};


class SeisTrcStorOutput : public Output
{
public:
				SeisTrcStorOutput(const CubeSampling&,
						  const LineKey&);
				~SeisTrcStorOutput();
    
    virtual bool		doInit();
    virtual void		set2D( bool yn = true )		{ is2d_ = yn; }
    bool			getDesiredVolume(CubeSampling&) const;
    bool			wantsOutput(const BinID&) const;
    bool			setStorageID(const MultiID&);
    void			setGeometry(const CubeSampling&);

    bool			doUsePar(const IOPar&);
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&);
    void			writeTrc();
    void			setOutpTypes(const TypeSet<Seis::DataType>& typ)
				{ outptypes_ = typ; }
    virtual TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    						TypeSet<float>&) const;

    static const char*		seisidkey;
    static const char*		attribkey;
    static const char*		inlrangekey;
    static const char*		crlrangekey;
    static const char*		depthrangekey;

protected:

    MultiID&			storid_;
    CubeSampling		desiredvolume_;
    TypeSet< Interval<int> >	sampleinterval_;
    IOPar*			auxpars_;
    bool			is2d_;
    BufferString 		errmsg_;

    SeisTrc*			trc_;
    SeisTrcWriter*		writer_;
    BinID 			prevpos_;
    bool			storinited_;
    BufferString		attribname_;
    Scaler*                     scaler_;
    TypeSet<Seis::DataType>	outptypes_;
};


class Trc2DVarZStorOutput : public SeisTrcStorOutput
{
public:
				Trc2DVarZStorOutput(const LineKey&,
						    const BinIDValueSet&,float);
    
    bool			doInit();
    void			set2D(bool)			{}

    void			setTrcsBounds(Interval<float>);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    					TypeSet<float>&) const;

    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&);
protected:

    const CubeSampling		getCS();

    const BinIDValueSet&	zvalues_;
    float			stdtrcsz_;
    float			stdstarttime_;
    float			outval_;
};


class TwoDOutput : public Output
{
public:
				TwoDOutput(const Interval<int>&, 
					   const Interval<float>&,
					   const LineKey&);
				~TwoDOutput();
    
    bool			doInit();
    bool			wantsOutput(const BinID&) const;
    void			setGeometry(const Interval<int>&,
	    				    const Interval<float>&);
    bool			getDesiredVolume(CubeSampling&) const;
    void			setOutput(Data2DHolder&);

    void			collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    					TypeSet<float>&) const;

protected:

    TypeSet< Interval<int> >	sampleinterval_;
    BufferString 		errmsg_;

    Data2DHolder*		output_;
};


class LocationOutput : public Output
{
public:
    				LocationOutput(BinIDValueSet&);
				~LocationOutput() {};

    bool			getDesiredVolume(CubeSampling&) const
    				{ return true;}
    bool			wantsOutput(const BinID&) const;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    					TypeSet<float>&) const;
    
    static const char*		filenamekey;
    static const char*		locationkey;
    static const char*		attribkey;
    static const char*		surfidkey;

protected:
    BinIDValueSet&		bidvalset_;
    int				classstatus_; 	// -1 Unknow 
    					      	//  0 Interpolate
    					      	//  1 Classification

    bool			arebiddupl_;	

    void			computeAndSetVals(const DataHolder&,
	    					  float,float*,float&,bool&);
    bool			areBIDDuplicated() const;
};


class TrcSelectionOutput : public Output
{
public:
    				TrcSelectionOutput(const BinIDValueSet&, 
						   float outval =0);
				~TrcSelectionOutput() ;

    bool			getDesiredVolume(CubeSampling&) const
				{ return true;}
    bool			wantsOutput(const BinID&) const;
    void			setOutput(SeisTrcBuf*);
    void			setTrcsBounds(Interval<float>);
    virtual void		collectData(const DataHolder&,float,
	    				    const SeisTrcInfo&);
    TypeSet< Interval<int> >	getLocalZRanges(const BinID&,float,
	    					TypeSet<float>&) const;
    
protected:
    const BinIDValueSet&	bidvalset_;
    SeisTrcBuf*			outpbuf_;
    float			outval_;
    float			stdtrcsz_;
    float			stdstarttime_;
};

} // namespace Attrib


#endif

#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.17 2005-09-29 11:29:41 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"
#include "seistrcsel.h"
#include "linekey.h"

class BinID;
class BinIDValueSet;
class MultiID;
class SeisSelData;
class SeisTrc;
class SeisTrcInfo;
class SeisTrcBuf;
class SeisTrcWriter;

namespace Attrib
{
class DataHolder;
class SliceSet;

class Output
{ mRefCountImpl(Output);
public:
    				Output(); 
    virtual bool		getDesiredVolume(CubeSampling&) const
    				{ return true; }
    virtual bool		wantsOutput( const BinID& ) const	= 0;
    virtual SliceSet*		getSliceSet() const { return 0; }
    virtual void		getDesiredOutputs( TypeSet<int>& outputs) 					const { outputs = desoutputs;}
    void			setDesiredOutputs( TypeSet<int> outputs )
    				{ desoutputs = outputs;}
    virtual TypeSet< Interval<int> >  getLocalZRange( const BinID& ) const = 0;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx)= 0;
    virtual void		writeTrc(){};
    const SeisSelData&		getSelData() { return seldata_; }

    TypeSet<int>		desoutputs;

    static const char*		outputstr;
    static const char*          cubekey;
    static const char*          tskey;
    static const char*          surfkey;
    static const char*          scalekey;

protected:
    SeisSelData&		seldata_;

};


class SliceSetOutput : public Output
{
public:
				SliceSetOutput(const CubeSampling&);
				~SliceSetOutput();

    SliceSet*			getSliceSet() const;

    bool			getDesiredVolume(CubeSampling&) const;
    void			setGeometry(const CubeSampling&);
    void                	setUndefValue( float v )	{ udfval = v; }

    bool			wantsOutput(const BinID&) const;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;
    
protected:
    CubeSampling		desiredvolume;
    TypeSet< Interval<int> >	sampleinterval;
    SliceSet*			sliceset;
    float			udfval;
};


class SeisTrcStorOutput : public Output
{
public:
				SeisTrcStorOutput(const CubeSampling&,LineKey);
				~SeisTrcStorOutput();
    
    bool			doInit();
    void			set2D()			{ is2d_=true; }
    bool			getDesiredVolume(CubeSampling&) const;
    bool			wantsOutput(const BinID&) const;
    bool			setStorageID(const MultiID&);
    void			setGeometry(const CubeSampling&);

    bool			doUsePar(const IOPar&);
    LineKey			curLineKey()	{ return seldata_.linekey_; }
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx);
    void			writeTrc();
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const
				{ return sampleinterval; }

    static const char*		seisidkey;
    static const char*		attribkey;
    static const char*		inlrangekey;
    static const char*		crlrangekey;
    static const char*		depthrangekey;

protected:

    MultiID&			storid_;
    CubeSampling		desiredvolume;
    TypeSet< Interval<int> >	sampleinterval;
    IOPar*			auxpars;
    bool			is2d_;
    BufferString 		errmsg;

    SeisTrc*			trc_;
    SeisTrcWriter*		writer_;
    BinID 			prevpos_;
    bool			storinited_;
};


class TwoDOutput : public Output
{
public:
				TwoDOutput(const Interval<int>&, 
					   const Interval<float>&,LineKey);
				~TwoDOutput() {};
    
    bool			doInit();
    bool			wantsOutput(const BinID&) const;
    void			setGeometry(const Interval<int>&,
	    				    const Interval<float>&);
    bool			getDesiredVolume(CubeSampling&) const;
    void			setOutput(ObjectSet<DataHolder>&,
	    				  ObjectSet<SeisTrcInfo>&);

    LineKey			curLineKey()	{ return seldata_.linekey_; }
    void			collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const
				{ return sampleinterval_; }

protected:

    TypeSet< Interval<int> >	sampleinterval_;
    BufferString 		errmsg;

    ObjectSet<DataHolder>*	datahset_;
    ObjectSet<SeisTrcInfo>*	trcinfoset_;
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
	    				    const SeisTrcInfo&,int outidx);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;
    
    static const char*		filenamekey;
    static const char*		locationkey;
    static const char*		attribkey;
    static const char*		surfidkey;

protected:
    BinIDValueSet&		bidvalset_;
};


class TrcSelectionOutput : public Output
{
public:
    				TrcSelectionOutput(const BinIDValueSet&, 
						   float outval =0,
						   Interval<float>* extraz =0);
				~TrcSelectionOutput() ;

    bool			getDesiredVolume(CubeSampling&) const
				{ return true;}
    bool			wantsOutput(const BinID&) const;
    void			setOutput(SeisTrcBuf*);
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;
    
protected:
    const BinIDValueSet&	bidvalset_;
    SeisTrcBuf*			outpbuf_;
    float			outval_;
    float			stdtrcsz_;
    float			stdstarttime_;
    Interval<float>*		extraz_;
};

} // namespace Attrib


#endif

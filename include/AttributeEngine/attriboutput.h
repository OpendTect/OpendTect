#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.12 2005-08-30 15:19:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"
#include "linekey.h"

class BinID;
class BinIDValueSet;
class MultiID;
class SeisSelData;
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
    virtual bool		getDesiredVolume(CubeSampling&) const	= 0;
    virtual bool		wantsOutput( const BinID& ) const	= 0;
    virtual SliceSet*		getSliceSet() const { return 0; }
    virtual SeisTrcBuf*         getTrcBuf() const { return 0; }
    virtual void		getDesiredOutputs( TypeSet<int>& outputs) 					const { outputs = desoutputs;}
    void			setDesiredOutputs( TypeSet<int> outputs )
    				{ desoutputs = outputs;}
    virtual TypeSet< Interval<int> >  getLocalZRange( const BinID& ) const = 0;
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx)= 0;
    virtual bool		setReqs(const BinID&) { return true; }
    const SeisSelData&		getSelData() { return seldata_; }

    TypeSet<int>		desoutputs;

    static const char*		outputstr;
    static const char*          typekey;
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


class CubeOutput : public Output
{
public:
				CubeOutput(const CubeSampling&,LineKey);
				~CubeOutput();
    
    bool			doInit();
    void			set2D()			{ is2d_=true; }
    bool			getDesiredVolume(CubeSampling&) const;
    bool			wantsOutput(const BinID&) const;
    bool			setStorageID(const MultiID&);
    void			setGeometry(const CubeSampling&);

    bool			doUsePar(const IOPar&);
    bool			setReqs(const BinID&);
    SeisTrcBuf* 		getTrcBuf() const;
    LineKey			curLineKey()		{ return lkey_; }
    virtual void		collectData(const DataHolder&,float step,
	    				    const SeisTrcInfo&,int outidx);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;

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

    SeisTrcWriter*		writer_;
    bool 			calcurpos_;
    BinID 			prevpos_;
    bool			storinited_;
    SeisTrcBuf*			buf2d_;
    LineKey			lkey_;
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
    				TrcSelectionOutput(const BinIDValueSet&);
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
    float			stdtrcsz_;
    float			stdstarttime_;
};

} // namespace Attrib


#endif

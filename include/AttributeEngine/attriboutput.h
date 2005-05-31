#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.5 2005-05-31 12:50:09 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"
#include "attribdataholder.h"
#include "seistrcsel.h"
#include "linekey.h"
#include "binidvalset.h"

class BinID;
class MultiID;
class SeisTrcWriter;
class SeisTrcBuf;

namespace Attrib
{
class SliceSet;

class Output
{ mRefCountImpl(Output);
public:
    				Output(); 
    virtual bool		getDesiredVolume(CubeSampling&) const	= 0;
    virtual bool		wantsOutput( const BinID& ) const	= 0;
    virtual void		getDesiredOutputs( TypeSet<int>& outputs) 					const { outputs = desoutputs;}
    void			setDesiredOutputs( TypeSet<int> outputs )
    				{ desoutputs = outputs;}
    virtual Interval<int>	getLocalZRange( const BinID& ) const	= 0;
    virtual void		collectData( const BinID&,const DataHolder&, 
	    					float, int )	= 0;
    virtual bool		setReqs(const BinID&) { return true; }
    const SeisSelData&		getSelData() { return seldata_; }

    TypeSet<int>		desoutputs;

protected:
    SeisSelData&		seldata_;

};


class SliceSetOutput : public Output
{
public:
    			SliceSetOutput( const CubeSampling& );
			~SliceSetOutput();

    SliceSet*		getSliceSet() const;

    bool		getDesiredVolume(CubeSampling&) const;
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange(const BinID&) const;
    void		collectData(const BinID&, const DataHolder&,float,int);
protected:
    CubeSampling	desiredvolume;
    Interval<int>	sampleinterval;
    SliceSet*		sliceset;
};


class CubeOutput : public Output
{
public:
    			CubeOutput( const CubeSampling&, LineKey );
			~CubeOutput();
    
    bool		doInit();
    bool		getDesiredVolume( CubeSampling& ) const;
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange( const BinID& ) const;
    bool		setStorageID( const MultiID& );
    void		setGeometry( const CubeSampling& );

    bool		doUsePar( const IOPar& );
    bool		setReqs(const BinID&);
    SeisTrcBuf* 	getTrcBuf();
    LineKey		curLineKey() { return lkey_; }
    void		collectData( const BinID&, const DataHolder&, 
	    				float, int );

protected:

    MultiID&            storid_;
    CubeSampling	desiredvolume;
    Interval<int>	sampleinterval;
    IOPar*              auxpars;
    bool		is2d_;
    BufferString 	errmsg;

    SeisTrcWriter*      writer_;
    bool 		calcurpos_;
    BinID 		prevpos_;
    bool		storinited_;
    SeisTrcBuf*         buf2d_;
    LineKey		lkey_;
};


class LocationOutput : public Output
{
public:
    			LocationOutput( BinIDValueSet& );
			~LocationOutput() {};

    bool		getDesiredVolume(CubeSampling&) const	{ return true;}
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange(const BinID&) const;
    void		collectData(const BinID&, const DataHolder&,float,int);
protected:
    Interval<int>	sampleinterval;
    BinIDValueSet	bidvalset_;
};


class TrcSelectionOutput : public Output
{
public:
    			TrcSelectionOutput(const char* blablaclass);
			~TrcSelectionOutput() {};

    bool		getDesiredVolume(CubeSampling&) const	{ return true;}
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange(const BinID&) const;
    void		collectData(const BinID&, const DataHolder&,float,int);
protected:
    Interval<int>	sampleinterval;
};

}; //Namespace


#endif

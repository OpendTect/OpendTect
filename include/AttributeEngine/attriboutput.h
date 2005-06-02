#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.6 2005-06-02 10:37:53 cvshelene Exp $
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
    virtual TypeSet< Interval<int> >  getLocalZRange( const BinID& ) const = 0;
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
    void		collectData(const BinID&, const DataHolder&,float,int);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;
    
protected:
    CubeSampling		desiredvolume;
    TypeSet< Interval<int> >	sampleinterval;
    SliceSet*			sliceset;
};


class CubeOutput : public Output
{
public:
    			CubeOutput( const CubeSampling&, LineKey );
			~CubeOutput();
    
    bool		doInit();
    bool		getDesiredVolume( CubeSampling& ) const;
    bool		wantsOutput( const BinID& ) const;
    bool		setStorageID( const MultiID& );
    void		setGeometry( const CubeSampling& );

    bool		doUsePar( const IOPar& );
    bool		setReqs(const BinID&);
    SeisTrcBuf* 	getTrcBuf();
    LineKey		curLineKey() { return lkey_; }
    void		collectData( const BinID&, const DataHolder&, 
	    				float, int );
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;

protected:

    MultiID&            	storid_;
    CubeSampling		desiredvolume;
    TypeSet< Interval<int> >	sampleinterval;
    IOPar*              	auxpars;
    bool			is2d_;
    BufferString 		errmsg;

    SeisTrcWriter*      	writer_;
    bool 			calcurpos_;
    BinID 			prevpos_;
    bool			storinited_;
    SeisTrcBuf*         	buf2d_;
    LineKey			lkey_;
};


class LocationOutput : public Output
{
public:
    			LocationOutput( BinIDValueSet& );
			~LocationOutput() {};

    bool		getDesiredVolume(CubeSampling&) const	{ return true;}
    bool		wantsOutput( const BinID& ) const;
    void		collectData(const BinID&, const DataHolder&,float,int);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;

protected:
    TypeSet< Interval<int> >	sampleinterval;
    BinIDValueSet		bidvalset_;
};


class TrcSelectionOutput : public Output
{
public:
    			TrcSelectionOutput(const BinIDValueSet& );
			~TrcSelectionOutput() ;

    bool		getDesiredVolume(CubeSampling&) const	{ return true;}
    bool		wantsOutput( const BinID& ) const;
    void		collectData(const BinID&, const DataHolder&,float,int);
    TypeSet< Interval<int> >	getLocalZRange(const BinID&) const;
    
protected:
    TypeSet< Interval<int> >	sampleinterval;
    BinIDValueSet       	bidvalset_;
    SeisTrcBuf*         	outpbuf_;
    float 			stdtrcsz_;
    float			stdstarttime_;
};

}; //Namespace


#endif

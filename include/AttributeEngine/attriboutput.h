#ifndef attriboutput_h
#define attriboutput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriboutput.h,v 1.2 2005-05-09 14:40:01 cvshelene Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "ranges.h"
#include "refcount.h"
#include "attribdataholder.h"
#include "seistrcsel.h"

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
    				Output() {mRefCountConstructor;}
    virtual bool		getDesiredVolume(CubeSampling&) const	= 0;
    virtual bool		wantsOutput( const BinID& ) const	= 0;
    virtual void		getDesiredOutputs( TypeSet<int>& outputs) 					const { outputs = desoutputs;}
    void			setDesiredOutputs( TypeSet<int> outputs )
    				{ desoutputs = outputs;}
    virtual Interval<int>	getLocalZRange( const BinID& ) const	= 0;
    virtual void		collectData( const BinID&,const DataHolder&, 
	    					float, int )	= 0;
    virtual bool		setReqs(const BinID&) { return true; }

    TypeSet<int>		desoutputs;
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


/*class Seis2DOutput : public Output
{
public:
			Seis2DOutput(const CubeSampling&);
			~Seis2DOutput();

    SeisTrcBuf*		getTrcBuf() const;

    bool                getDesiredVolume(CubeSampling&) const;
    bool                wantsOutput( const BinID& ) const;
    Interval<int>       getLocalZRange(const BinID&) const;
    void                collectData(const BinID&,const DataHolder&,float,int);

protected:
    CubeSampling        desiredvolume;
    Interval<int>       sampleinterval;
    Seis2DLineSet*      seis2dlineset;
};*/


class StorageOutput : public Output
{
public:
    			StorageOutput( const CubeSampling&, const char* );
			~StorageOutput();
    
    bool		getDesiredVolume( CubeSampling& ) const;
    bool		wantsOutput( const BinID& ) const;
    Interval<int>	getLocalZRange( const BinID& ) const;
    bool		setStorageID( const MultiID& );
    void		setGeometry( const CubeSampling& );
    virtual void	collectData( const BinID&, const DataHolder&, 
	    				float, int ) {};
    virtual bool	setReqs(const BinID&) { return true; }

protected:
    
    MultiID&            storid_;
    CubeSampling	desiredvolume;
    Interval<int>	sampleinterval;
    SeisSelData&        seldata;
    IOPar*              auxpars;
    bool		is2d_;
    BufferString 	errmsg;
};

class CubeOutput : public StorageOutput
{
public:
    			CubeOutput( const CubeSampling&, const char* lk);
			~CubeOutput();
    
    bool		doInit();

    bool		doUsePar( const IOPar& );
    bool		setReqs(const BinID&);
    SeisTrcBuf* 	getTrcBuf();
    const char*		curLineName() { return lk_; }
    void		collectData( const BinID&, const DataHolder&, 
	    				float, int );

protected:

    SeisTrcWriter*      writer_;
    bool 		calcurpos_;
    BinID 		prevpos_;
    bool		storinited_;
    SeisTrcBuf*         buf2d_;
    const char*		lk_;
};

}; //Namespace


#endif

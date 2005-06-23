#ifndef attribengman_h
#define attribengman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id:
________________________________________________________________________

-*/

#include "sets.h"
#include "bufstring.h"
#include <iosfwd>

class BinID;
class IOPar;
class MultiID;
class LineKey;
class NLAModel;
class SeisTrcBuf;
class CubeSampling;
class PickSetGroup;
class BinIDValueSet;
class NLACreationDesc;//?
class BufferStringSet;
class ExecutorGroup;
template <class T> class TypeSet;
template <class T> class Interval;
template <class T> class ObjectSet;
template <class T> class StepInterval;

namespace Attrib
{
class Processor;
class DescSet;
class SelSpec;
class SliceSet;
class Output;
class CubeOutput;

/*!\brief The Attribute engine Manager. */

class EngineMan
{
public:
			EngineMan();
    virtual		~EngineMan();

    void		usePar( const IOPar& iopar, 
	    			const DescSet& attribset,
	    			const char* linename,
				ObjectSet<Processor>& procset ); 

    void		createProcSet( ObjectSet<Processor>& procset,
	    				const DescSet& attribset,
					const char* linename, TypeSet<int> );
    CubeOutput* 	createOutput( const IOPar& pars, LineKey ); 
    const DescSet* 	attribSet() const	{ return inpattrset; }
    const NLAModel*	nlaModel() const	{ return nlamodel; }
    const SelSpec& 	attribSpec() const	{ return attrspec; }
    const CubeSampling&	cubeSampling() const	{ return cs; }
    const BufferString&	lineKey() const		{ return linekey; }
    float		undefValue() const	{ return udfval; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setCubeSampling(const CubeSampling&);
    void		setLineKey( const char* lk )	{ linekey = lk; }
    void		setUndefValue( float v )	{ udfval = v; }
    void		addOutputAttrib(int);
    DescSet*		createNLAADS(int& outid,BufferString& errmsg,
	    			     const DescSet* addtoset=0);

    ExecutorGroup*	sliceSetOutputCreator(BufferString& errmsg,
	    			      const SliceSet* cached_data = 0);
    			//!< Give the previous calculated data in cached data
    			//!< and some parts may not be recalculated.
    SliceSet*		getSliceSetOutput();
    			//!< Mem transfer here
    SeisTrcBuf*		get2DLineOutput();
    			//!< Mem transfer here

    ExecutorGroup* 	featureOutputCreator(const BufferStringSet& inputs,
					const ObjectSet<BinIDValueSet>& );

    ExecutorGroup*	screenOutput2DCreator( BufferString& errmsg );
    ExecutorGroup*	locationOutputCreator(BufferString& errmsg,
					   ObjectSet<BinIDValueSet>&);

    ExecutorGroup*	trcSelOutputCreator(BufferString& errmsg,
	    				    const BinIDValueSet& bidvalset,
	    				    SeisTrcBuf&);

    void		prOut(std::ostream&,int);
    const char*		curUserDesc() const;

protected:

    const DescSet* 	inpattrset;
    const NLAModel*	nlamodel;
    CubeSampling&	cs;
    SliceSet*		cache;
    int			attrid2;
    float		udfval;
    BufferString	linekey;

    const DescSet* 	curattrset;
    DescSet*		procattrset;
    int			curattrid;
    TypeSet<int>	outattribs;
    SelSpec&      	attrspec;

    bool		getProcessors( ObjectSet<Processor>, 
	    				BufferString& errmsg, bool, 
	    				bool addcurid = true );
    BufferString	createExecutorName();

private:

    friend class		AEMFeatureExtracter;

    ObjectSet<Processor> 	procset;
    void			clearProcessing();
    void			clearZPtrs();

};

};//namespace


#endif

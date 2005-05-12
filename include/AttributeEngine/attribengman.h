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
class NLAModel;
class FeatureSet;//?
class SeisTrcBuf;
class CubeSampling;
class PickSetGroup;
class BinIDValueSet;
class NLACreationDesc;//?
class BufferStringSet;
template <class T> class TypeSet;
template <class T> class Interval;
template <class T> class ObjectSet;
template <class T> class StepInterval;

namespace Attrib
{
class Processor;
class DescSet;
class SliceSet;
class Output;
class CubeOutput;

/*!\brief The Attribute engine Manager. */

class EngineMan
{
public:
			EngineMan();
    virtual		~EngineMan();

    Processor*		usePar( const IOPar& iopar, const DescSet& attribset,
	    			const char* linename ); 

    CubeOutput* 	createOutput( const IOPar& pars, const char* ); 
    //old attribengman: voir si on garde toutes ces fonctions 
    //et les adapter le cas echeant.
/*    const AttribDescSet* attribSet() const	{ return inpattrset; }
    const NLAModel*	nlaModel() const	{ return nlamodel; }
    const AttribSelSpec& attribSpec() const	{ return attrspec; }
    const CubeSampling&	cubeSampling() const	{ return cs; }
    const BufferString&	lineKey() const		{ return linekey; }
    const MultiID&	outputID() const	{ return outid; }
    			//!< Set this to "" for in-mem output
    int			secondAttribID() const	{ return attrid2; }
    float		undefValue() const	{ return udfval; }

    void		setAttribSet(const AttribDescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const AttribSelSpec&);
    void		setCubeSampling(const CubeSampling&);
    void		setLineKey( const char* lk )	{ linekey = lk; }
    void		setOutputID(const MultiID&);
    void		setSecondAttribID( int i )	{ attrid2 = i; }
    void		setUndefValue( float v )	{ udfval = v; }
    void		addOutputAttrib(int);
    AttribDescSet*	createNLAADS(int& outid,BufferString& errmsg,
	    			     const AttribDescSet* addtoset=0);

    Executor*		cubeOutputCreater(BufferString& errmsg,
	    			      const AttribSliceSet* cached_data = 0);
    			//!< Give the previous calculated data in cached data
    			//!< and some parts may not be recalculated.
    AttribSliceSet*	getCubeOutput(Executor*);
    			//!< Mem transfer here
    			//!< Will return null if outputID set.
    SeisTrcBuf*		get2DLineOutput(Executor*);
    			//!< Mem transfer here
    			//!< Will return null if outputID set.

    Executor*		featureOutputCreator(const BufferStringSet& inputs,
					const ObjectSet<BinIDValueSet>&,
					    ObjectSet<FeatureSet>&);

    Executor*		tableOutputCreator(BufferString& errmsg,
					   ObjectSet<BinIDValueSet>&);

    Executor*		trcSelOutputCreator(BufferString& errmsg,
	    				    const TypeSet<BinID>&,
					    const Interval<float>&,
	    				    SeisTrcBuf&);

    void		prOut(std::ostream&,int);
    const char*		curUserDesc() const;
*/
protected:

    const DescSet* 	inpattrset;
    const NLAModel*	nlamodel;
    CubeSampling&	cs;
    int			attrid2;
    float		udfval;
    BufferString	linekey;

    const DescSet* curattrset;
    DescSet*	procattrset;
    int			curattrid;
    MultiID&		outid;
    TypeSet<int>	outattribs;

//    bool		mkOutput();
//    AttribOutputExecutor* mkOutputExecutor(BufferString&,bool needid=true);
//    AttribOutputExecutor* getOutputExecutor(BufferString&,bool needid=true);
//    void		setExecutorName(Executor*);

private:

//    friend class	AEMFeatureExtracter;

    void			clearProcessing();
//    void		clearZPtrs();

//    bool		handleAttribWithoutInputs();

};

};//namespace


#endif

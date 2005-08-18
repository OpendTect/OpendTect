#ifndef attribengman_h
#define attribengman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id: attribengman.h,v 1.8 2005-08-18 14:19:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "bufstring.h"
#include "attribdescid.h"

class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class ExecutorGroup;
class IOPar;
class LineKey;
class NLAModel;
class SeisTrcBuf;

namespace Attrib
{
class CubeOutput;
class Desc;
class DescSet;
class SelSpec;
class SliceSet;
class Processor;

/*!\brief The Attribute engine Manager. */

class EngineMan
{
public:
			EngineMan();
    virtual		~EngineMan();

    void		usePar(const IOPar&,const DescSet&,
	    		       const char* linename,
			       ObjectSet<Processor>&); 

    static void		createProcSet(ObjectSet<Processor>& procset,
				      const DescSet& attribset,
				      const char* linename,
				      const TypeSet<DescID>&);
    static void		getPossibleVolume(const DescSet&,CubeSampling&,
	    				  const char* linename,const DescID&);
    CubeOutput* 	createOutput(const IOPar&,const LineKey&);

    const DescSet* 	attribSet() const	{ return inpattrset; }
    const NLAModel*	nlaModel() const	{ return nlamodel; }
    const SelSpec& 	attribSpec() const	{ return attrspec; }
    const CubeSampling&	cubeSampling() const	{ return cs_; }
    const BufferString&	lineKey() const		{ return linekey; }
    float		undefValue() const	{ return udfval; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setCubeSampling(const CubeSampling&);
    void		setLineKey( const char* lk )	{ linekey = lk; }
    void		setUndefValue( float v )	{ udfval = v; }
    void		addOutputAttrib(const DescID&);
    DescSet*		createNLAADS(DescID& outid,BufferString& errmsg,
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
					     const ObjectSet<BinIDValueSet>&);

    ExecutorGroup*	screenOutput2DCreator(BufferString& errmsg);
    ExecutorGroup*	locationOutputCreator(BufferString& errmsg,
					      ObjectSet<BinIDValueSet>&);

    ExecutorGroup*	trcSelOutputCreator(BufferString& errmsg,
	    				    const BinIDValueSet& bidvalset,
	    				    SeisTrcBuf&);

    const char*		curUserDesc() const;

protected:

    const DescSet* 	inpattrset;
    const NLAModel*	nlamodel;
    CubeSampling&	cs_;
    SliceSet*		cache;
    float		udfval;
    BufferString	linekey;

    const DescSet* 	curattrset;
    DescSet*		procattrset;
    DescID		curattrid;
    TypeSet<DescID>	outattribs;
    SelSpec&      	attrspec;

    bool		getProcessors(ObjectSet<Processor>&,BufferString& err,
	    			      bool need_selspecid);
    BufferString	createExecutorName() const;
    ExecutorGroup*	createExecutorGroup() const;

private:

    friend class		AEMFeatureExtracter;

    ObjectSet<Processor> 	procset;
    void			clearZPtrs();

};

};//namespace


#endif

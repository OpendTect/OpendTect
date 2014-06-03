#ifndef attribengman_h
#define attribengman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "sets.h"
#include "ranges.h"
#include "bufstring.h"
#include "attribdescid.h"
#include "attribsel.h"
#include "uistring.h"

class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class DataPointSet;
class Executor;
class LineKey;
class NLAModel;
class SeisTrcBuf;
class SeisTrcInfo;


namespace Attrib
{
class SeisTrcStorOutput;
class Desc;
class DescSet;
class DataCubes;
class Processor;
class Data2DHolder;

/*!
\brief The attribute engine manager.
*/

mExpClass(AttributeEngine) EngineMan
{ mODTextTranslationClass(Attrib::EngineMan);
public:
			EngineMan();
    virtual		~EngineMan();

    Processor*		usePar(const IOPar&,DescSet&,
			       const char* linename,uiString&);

    static Processor*	createProcessor(const DescSet&,const char*,
					const DescID&,uiString& errmsg);
    static void		getPossibleVolume(DescSet&,CubeSampling&,
					  const char* linename,const DescID&);
    static void		addNLADesc(const char*,DescID&,DescSet&,int,
				   const NLAModel*,uiString&);

    SeisTrcStorOutput*	createOutput(const IOPar&,const LineKey&,uiString&);

    const DescSet*	attribSet() const	{ return inpattrset_; }
    const NLAModel*	nlaModel() const	{ return nlamodel_; }
    const CubeSampling&	cubeSampling() const	{ return cs_; }
    const OD::String&	lineKey() const		{ return linekey_; }
    float		undefValue() const	{ return udfval_; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setAttribSpecs(const TypeSet<SelSpec>&);
    void		setCubeSampling(const CubeSampling&);
    void		setLineKey( const char* lk )	{ linekey_ = lk; }
    void		setUndefValue( float v )	{ udfval_ = v; }
    DescSet*		createNLAADS(DescID& outid,uiString& errmsg,
				     const DescSet* addtoset=0);
    static DescID	createEvaluateADS(DescSet&, const TypeSet<DescID>&,
					  uiString&);

    Processor*		createDataCubesOutput(uiString& errmsg,
					      const DataCubes* cached_data = 0);
			//!< Give the previous calculated data in cached data
			//!< and some parts may not be recalculated.
    const DataCubes*	getDataCubesOutput(const Processor&);

    Executor*		createFeatureOutput(const BufferStringSet& inputs,
					    const ObjectSet<BinIDValueSet>&);

    Processor*		createScreenOutput2D(uiString& errmsg,
					     Data2DHolder&);
    Processor*		createLocationOutput(uiString& errmsg,
					     ObjectSet<BinIDValueSet>&);

    Processor*		createTrcSelOutput(uiString& errmsg,
					   const BinIDValueSet& bidvalset,
					   SeisTrcBuf&, float outval=0,
					   Interval<float>* cubezbounds=0,
					   TypeSet<BinID>* trueknotspos=0,
					   TypeSet<BinID>* path=0);
    Processor*		create2DVarZOutput(uiString& errmsg,
					   const IOPar& pars,
					   DataPointSet* bidvalset,
					   float outval=0,
					   Interval<float>* cubezbounds = 0);
    Processor*		getTableOutExecutor(DataPointSet& datapointset,
					    uiString& errmsg,
					    int firstcol);
    Executor*		getTableExtractor(DataPointSet&,const Attrib::DescSet&,
					  uiString& errmsg,int firstcol =0,
					  bool needprep=true);
    static bool		ensureDPSAndADSPrepared(DataPointSet&,
						const Attrib::DescSet&,
						uiString& errmsg);
    int			getNrOutputsToBeProcessed(const Processor&) const;

    const char*		getCurUserRef() const;
    void		computeIntersect2D(ObjectSet<BinIDValueSet>&) const;

protected:

    const DescSet*	inpattrset_;
    const NLAModel*	nlamodel_;
    CubeSampling&	cs_;
    const DataCubes*	cache_;
    float		udfval_;
    BufferString	linekey_;

    DescSet*		procattrset_;
    int			curattridx_;
    TypeSet<SelSpec>	attrspecs_;

    Processor*		getProcessor(uiString& err);
    void		setExecutorName(Executor*);

private:

    friend class		AEMFeatureExtracter;//TODO will soon be removed
    friend class		AEMTableExtractor;

    void			clearZPtrs();

};

};//namespace


#endif


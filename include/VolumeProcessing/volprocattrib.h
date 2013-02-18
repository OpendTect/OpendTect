#ifndef volprocattrib_h
#define volprocattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
//#include "attribprovider.h"
#include "attribparam.h"
#include "externalattrib.h"
#include "multiid.h"


namespace VolProc
{
class Chain;
class ChainExecutor;
/*
mExpClass(VolumeProcessing) AttributeAdapter : public Attrib::Provider
{
public:
    static void		initClass();
    			AttributeAdapter(Attrib::Desc&);

    static const char*	attribName()		{ return "Volume_Processing"; }
    static const char*	sKeyRenderSetup()	{ return "volprocsetup"; }

    bool		isOK() const;

protected:
    				~AttributeAdapter();
    static Attrib::Provider*	createInstance(Attrib::Desc&);

    bool			allowParallelComputation() const {return false;}
    bool			computeData(const Attrib::DataHolder&,
	    				    const BinID&,int t0,
					    int nrsamples,int threadid) const;

    void			prepareForComputeData();
    bool			prepareChain();

    Chain*		chain_;
    MultiID			rendermid_;
    bool			firstlocation_;
    ChainExecutor*	executor_;
};

*/

/*!
\brief Adapter for a VolProc chain to external attribute calculation.
*/

mExpClass(VolumeProcessing) ExternalAttribCalculator : public Attrib::ExtAttribCalc
{
public:
    static void		initClass();
    			ExternalAttribCalculator();
    			~ExternalAttribCalculator();

    static const char*	sAttribName()	{ return "Volume_Processing"; }
    static const char*	sKeySetup()	{ return "volprocsetup"; }

    static BufferString	createDefinition(const MultiID& setup);

    bool		setTargetSelSpec(const Attrib::SelSpec&);

    virtual DataPack::ID createAttrib(const CubeSampling&,DataPack::ID,
	    			     TaskRunner*);
    virtual bool	createAttrib( ObjectSet<BinIDValueSet>& o,
	    			      TaskRunner* tr )
			{ return Attrib::ExtAttribCalc::createAttrib(o,tr); }
    virtual bool	createAttrib( const BinIDValueSet& b, SeisTrcBuf& tb,
	    			      TaskRunner* tr )
			{ return Attrib::ExtAttribCalc::createAttrib(b,tb,tr); }
    virtual DataPack::ID createAttrib( const CubeSampling& cs, const LineKey& l,
					TaskRunner* tr )
			{ return Attrib::ExtAttribCalc::createAttrib(cs,l,tr); }

protected:

    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

    Chain*			chain_;
    MultiID			rendermid_;

};

}; //namespace

#endif


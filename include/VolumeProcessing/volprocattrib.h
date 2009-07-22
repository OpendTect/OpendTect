#ifndef volprocattrib_h
#define volprocattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: volprocattrib.h,v 1.6 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

//#include "attribprovider.h"
#include "attribparam.h"
#include "externalattrib.h"
#include "multiid.h"


namespace VolProc
{
class Chain;
class ChainExecutor;
/*
mClass AttributeAdapter : public Attrib::Provider
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

/*!Adapter for a VolProc chain to external attribute calculation */

mClass ExternalAttribCalculator : public Attrib::ExtAttribCalc
{
public:
    static void			initClass();
    				ExternalAttribCalculator();
    				~ExternalAttribCalculator();

    static const char*		sAttribName()	{ return "Volume_Processing"; }
    static const char*		sKeySetup()	{ return "volprocsetup"; }

    static BufferString		createDefinition(const MultiID& setup);

    bool			setTargetSelSpec(const Attrib::SelSpec&);
    DataPack::ID		createAttrib(const CubeSampling&,DataPack::ID,
	    				     TaskRunner*);

protected:
    static Attrib::ExtAttribCalc* create(const Attrib::SelSpec&);

    Chain*			chain_;
    MultiID			rendermid_;
};

}; //namespace

#endif

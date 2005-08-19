#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.10 2005-08-19 07:17:53 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "cubesampling.h"
#include "seisreq.h"

namespace Attrib
{

class DataHolder;

class StorageProvider : public Provider
{
public:
    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    bool		init();

    int			moveToNextTrace();
    bool		getPossibleVolume(int outp,CubeSampling&);
    BinID		getStepoutStep() const;
    void		updateStorageReqs(bool all=true);
    void		adjust2DLineStoredVolume();


protected:
    			~StorageProvider();
    static Provider*	createFunc(Desc&);
    static void		updateDesc(Desc&);

    			StorageProvider(Desc&);
    SeisRequester*	getSeisRequester() const;

    bool		initSeisRequester(int req);
    bool		setSeisRequesterSelection(int req);
    void		setBufferStepout(const BinID&);

    bool        	computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples) const;

    void		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume.zrg.step; return true; }
    
    bool 		checkDataOK( StepInterval<int> trcrg,
	                             StepInterval<float>zrg );
    bool 		checkDataOK();

    SeisReqGroup	rg;
    int			currentreq;

    CubeSampling	storedvolume;

    enum Status        { Nada, StorageOpened, Ready } status;
};

}; // namespace Attrib

#endif

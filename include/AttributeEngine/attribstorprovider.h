#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.5 2005-05-27 07:28:42 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "attribdataholder.h"

#include "cubesampling.h"
#include "seisreq.h"

namespace Attrib
{

class StorageProvider : public Provider
{
public:
    static void		initClass();

    bool		init();

    int			moveToNextTrace();
    bool		getPossibleVolume(int outp,CubeSampling&);
    static const char*  attribName() { return "Storage"; }
    static const char*  keyStr() { return "id"; }
    BinID		getStepoutStep();
    void		updateStorageReqs();
    void		adjust2DLineStoredVolume();


protected:
    			~StorageProvider();
    static Provider*	createFunc( Desc& );
    static void		updateDesc( Desc& );

    			StorageProvider( Desc& );
    SeisRequester*	getSeisRequester();

    bool		initSeisRequester(int req);
    bool		setSeisRequesterSelection(int req);

    bool        	computeData( const DataHolder& output,
	                	     const BinID& relpos,
				     int t0, int nrsamples ) const;

    void		fillDataHolderWithTrc( const SeisTrc*,
	    					const DataHolder& ) const;
    bool		getZStepStoredData(float& step) const
			    {step = storedvolume.zrg.step; return true;}

    SeisReqGroup	rg;
    int			currentreq;

    CubeSampling	storedvolume;

    enum Status        { Nada, StorageOpened, Ready } status;

};


}; //Namespace


#endif

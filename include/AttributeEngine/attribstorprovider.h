#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.2 2005-02-03 15:35:02 kristofer Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

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
    bool		getPossibleVolume(int outp,CubeSampling&) const;


protected:
    static Provider*	createFunc( Desc& );
    static void		updateDesc( Desc& );
    static const char*	attribName() { return "Storage"; }
    static const char*	keyStr() { return "key"; }

    			StorageProvider( Desc& );
    SeisRequester*	getSeisRequester();

    bool		initSeisRequester(int req);
    bool		setSeisRequesterSelection(int req);

    SeisReqGroup		rg;
    int				currentreq;

    CubeSampling		storedvolume;

    enum Status                 { Nada, StorageOpened, Ready } status;

};


}; //Namespace


#endif

#ifndef attribstorprovider_h
#define attribstorprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribstorprovider.h,v 1.1 2005-02-01 16:00:52 kristofer Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

namespace Attrib
{

class StorageProvider : public Provider
{
public:
    static void		initClass();

    int			moveToNextTrace();


protected:
    static Provider*	createFunc( Desc& );
    static bool		isDescOK( const Desc& );
    static void		updateDesc( Desc& );
    static const char*	attribName() { return "Storage"; }
    static const char*	keyStr() { return "key"; }
    			StorageProvider( Desc& );

    ObjectSet<SeisRequester>	rg;
    int				currentreq;

    enum Status                 { Nada, StorageOpened, Ready } status;

};


}; //Namespace


#endif

#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.1 2002-05-16 14:19:03 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "callback.h"
#include "bufstring.h"

class IOObj;
class Executor;

namespace EarthModel
{
class EMManager;

/*!\brief


*/

class EMObject : public CallBacker
{
public:
    static EMObject*		create( const IOObj&, bool load,
	    				EMManager&,
	    				BufferString& errmsg );

    				EMObject( EMManager&, int id );
    virtual			~EMObject( ) {}
    unsigned short		id() const { return id_; }
    const char*			name() const { return name_; }
    CNotifier<EMObject, PosID>	poschnotifier;

    virtual Executor*		loader() { return 0; }
    virtual bool		isloaded() const {return false;}
    virtual Executor*		saver() { return 0; }

    const char*			errMsg() const
    				{ return errmsg[0] ? errmsg : 0; }

protected:
    unsigned short		id_;
    class EMManager&		manager;
    BufferString		name_;

    BufferString		errmsg;
};

}; // Namespace


#endif

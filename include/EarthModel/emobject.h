#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.12 2003-07-11 08:15:46 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "callback.h"
#include "bufstring.h"
#include "multiid.h"

class IOObj;
class Executor;
struct CubeSampling;

namespace EM
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

    void			ref() const;
    void			unRef() const;
    void			unRefNoDel() const;

    				EMObject( EMManager&, const MultiID&);
    virtual			~EMObject( );
    const MultiID&		id() const { return id_; }
    BufferString		name() const;

    virtual Coord3		getPos(const EM::PosID&) const = 0;
    virtual bool		setPos(const EM::PosID&,
	    			       const Coord3&,
				       bool addtohistory ) = 0;

    virtual void		getLinkedPos( const EM::PosID& posid,
					  TypeSet<EM::PosID>& ) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/
    				
    				
    virtual void		setPosAttrib( EM::PosID&, int attr,
	   				      bool yn );
    virtual bool		isPosAttrib(EM::PosID&, int attr) const;

    CNotifier<EMObject, PosID>	poschnotifier;

    virtual Executor*		loader() { return 0; }
    virtual bool		isLoaded() const {return false;}
    virtual Executor*		saver() { return 0; }
    virtual bool		isChanged( int what=-1 ) const { return false; }
    				/*!<\param what	Says what the query is about.
						-1 mean any change, all other
						figures are dependent on impl.
				*/
				

    const char*			errMsg() const
    				{ return errmsg[0]
				    ? (const char*) errmsg : (const char*) 0; }

protected:
    MultiID			id_;
    class EMManager&		manager;
    BufferString		errmsg;

    ObjectSet<TypeSet<PosID> >	posattribs;
    TypeSet<int>		attribs;
};

}; // Namespace


#endif

#ifndef emobject_h
#define emobject_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emobject.h,v 1.10 2003-05-26 09:17:01 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "callback.h"
#include "bufstring.h"
#include "multiid.h"

class IOObj;
class Executor;
struct CubeSampling;

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

    void			ref() const;
    void			unRef() const;
    void			unRefNoDel() const;

    				EMObject( EMManager&, const MultiID&);
    virtual			~EMObject( );
    const MultiID&		id() const { return id_; }
    BufferString		name() const;

    virtual Coord3		getPos(const EarthModel::PosID&) const = 0;
    virtual bool		setPos(const EarthModel::PosID&,
	    			       const Coord3&,
				       bool addtohistory ) = 0;

    virtual void		getLinkedPos( const EarthModel::PosID& posid,
					  TypeSet<EarthModel::PosID>& ) const
    					{ return; }
    				/*!< Gives positions on the object that are
				     linked to the posid given
				*/
    				
    				
    virtual void		setPosAttrib( EarthModel::PosID&, int attr,
	   				      bool yn );
    virtual bool		isPosAttrib(EarthModel::PosID&, int attr) const;

    CNotifier<EMObject, PosID>	poschnotifier;

    virtual Executor*		loader() { return 0; }
    virtual bool		isLoaded() const {return false;}
    virtual Executor*		saver() { return 0; }

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

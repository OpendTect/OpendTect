#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdata.h,v 1.34 2005-01-07 11:23:15 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase { class DataObject; class EventInfo; }
typedef visBase::DataObject* (*FactPtr)(void);


#define mVisTrans visBase::Transformation


namespace visBase
{
class Transformation;
class SelectionManager;
class DataManager;
class Scene;

/*!\brief
DataObject is the base class off all objects that are used in Visualisation and
ought to be shared in visBase::DataManager. The DataManager owns all the
objects and is thus the only one that is allowed to delete it. The destructors
on the inherited classes should thus be protected.
*/

class DataObject : public CallBackClass
{
public:

    virtual const char*		getClassName() const { return "Not impl"; }

    int				id() const { return id_; }
    const char*			name() const;
    void			setName( const char* );

    virtual SoNode*		getInventorNode() { return 0; }
    virtual const SoNode*	getInventorNode() const;

    void			ref() const;
    void			unRef() const;
    void			unRefNoDelete() const;

    virtual bool		selectable() const { return false; }
    void			select() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			deSelect() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    bool			isSelected() const;
    virtual NotifierAccess*	selection() 		{ return 0; }
    virtual NotifierAccess*	deSelection() 		{ return 0; }
    virtual NotifierAccess*	rightClicked()		{ return 0; }
    virtual const TypeSet<int>*	rightClickedPath() const{ return 0; }

    virtual void		setDisplayTransformation( Transformation* );
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual Transformation*	getDisplayTransformation() { return 0; }
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */

    virtual int			usePar( const IOPar& );
    				/*!< Returns -1 on error and 1 on success.
				     If it returns 0 it is missing something.
				     Parse everything else and retry later.
				*/

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
			
    static const char*		typestr;
    static const char*		namestr;

    bool			dumpOIgraph( const char* filename,
	    				     bool binary=false);

protected:
    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()		{}
    virtual void		triggerDeSel()		{}
    virtual void		triggerRightClick(const EventInfo* =0) {}
    
				DataObject();
    virtual			~DataObject();
    virtual void		_init();

    friend class		DataManager;
    void			remove() { delete this; }
    				/* Should only be called by DataManager */
private:
    int				id_;
    BufferString*		name_;

};


class Factory;

class FactoryEntry : public CallBackClass
{
public:
    			FactoryEntry( FactPtr, const char*);
    			~FactoryEntry();

    DataObject*		create(void) { return (*funcptr)(); }

    			FactPtr funcptr;
    const char*		name;

protected:
    void		visIsClosingCB(CallBacker*);
    visBase::Factory*	factory;
};


class Factory : public CallBacker
{
public:
    				Factory(); 
    				~Factory();
    DataObject*			create( const char* );
    ObjectSet<FactoryEntry>	entries;

    Notifier<Factory>		closing;
};

};

#define _mCreateDataObj(clss,args) 				\
{								\
    clss* res = new clss args;					\
    res->_init();						\
    return res;							\
}								\
private:							\
    clss(const clss&);						\
    clss& operator =(const clss&);				\
    static visBase::FactoryEntry	__factoryentry;		\
public:								\
    virtual const char*	getClassName() const 			\
	{ return __factoryentry.name; }				\
protected:
    
#define _mDeclConstr(clss)	\
    clss();			\
public:

#define mCreateDataObj(clss) \
    _mCreateDataObj(clss,()) \
    _mDeclConstr(clss)

#define mCreateFactoryEntry( clss )				\
    visBase::FactoryEntry clss::__factoryentry(			\
    (FactPtr) clss::create, #clss)


#endif

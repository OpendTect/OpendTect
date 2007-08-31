#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdata.h,v 1.46 2007-08-31 12:48:58 cvskris Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "refcount.h"
#include "sets.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase { class DataObject; class EventInfo; }


#define mVisTrans visBase::Transformation


namespace visBase
{

typedef DataObject* (*FactPtr)(void);

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

class DataObject : public CallBacker
{ mRefCountImpl(DataObject);
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    virtual bool		isOK() const		{ return true; }

    int				id() const		{ return id_; }
    void			setID(int nid)		{ id_= nid; }

    const char*			name() const;
    void			setName(const char*);

    virtual SoNode*		getInventorNode()	{ return 0; }
    virtual const SoNode*	getInventorNode() const;

    virtual bool		selectable() const	{ return false; }
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

    virtual void		setDisplayTransformation(Transformation*);
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

    virtual int			usePar(const IOPar&);
    				/*!< Returns -1 on error and 1 on success.
				     If it returns 0 it is missing something.
				     Parse everything else and retry later.
				*/

    virtual bool		acceptsIncompletePar() const    {return false;}
				/*!<Returns true if it can cope with non-
				    complete session-restores. If it returns
				    false, DataManager::usePar() will
				    fail if usePar of this function returns
				    0, and it doesn't help to retry.  */

    virtual void		fillPar(IOPar&, TypeSet<int>&) const;
			
    static const char*		sKeyType();
    static const char*		sKeyName();

    bool			dumpOIgraph(const char* filename,
	    				    bool binary=false);

protected:
    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()				{}
    				/*!<Is called everytime object is selected.*/
    virtual void		triggerDeSel()				{}
    				/*!<Is called everytime object is deselected.*/
    virtual void		triggerRightClick(const EventInfo* =0)	{}
    
				DataObject();
    virtual bool		_init();

private:
    int				id_;
    BufferString*		name_;

};


class Factory;

/*! The FactoryEntry knows how to create one visualization object, and it has a 
    function that can produce the object.  */

class FactoryEntry : public CallBacker
{
public:
    			FactoryEntry( FactPtr, const char*);
    			~FactoryEntry();

    DataObject*		create();
    const char*		name() const { return name_; }

protected:
    void		visIsClosingCB(CallBacker*);

    visBase::Factory*	factory_;
    FactPtr		funcptr_;
    const char*		name_;
};


class Factory : public CallBacker
{
public:
    				Factory(); 
    				~Factory();

    void			addEntry(FactoryEntry*);
    void			removeEntry(FactoryEntry*);

    DataObject*			create(const char*);
    FactoryEntry*		getEntry(const char*);

    Notifier<Factory>		closing;

protected:
    ObjectSet<FactoryEntry>	entries_;

};

};

#define _mCreateDataObj(clss) 					\
{								\
    clss* res = (clss*) factoryentry_.create();			\
    return res;							\
}								\
								\
private:							\
    static visBase::DataObject* createInternal()		\
    {								\
	clss* res = new clss;					\
	if ( !res )						\
	    return 0;						\
	if ( !res->_init() || !res->isOK() )			\
	{							\
	    delete res;						\
	    return 0;						\
	}							\
								\
	return res;						\
    }								\
								\
    clss(const clss&);						\
    clss& operator =(const clss&);				\
    static visBase::FactoryEntry	factoryentry_;		\
public:								\
    static const char* getStaticClassName()			\
	{ return factoryentry_.name(); }			\
								\
    virtual const char*	getClassName() const 			\
	{ return getStaticClassName(); }			\
protected:
    
#define _mDeclConstr(clss)	\
    clss();			\
public:

#define mCreateDataObj(clss) \
    _mCreateDataObj(clss) \
    _mDeclConstr(clss)

#define mCreateFactoryEntry( clss )				\
    visBase::FactoryEntry clss::factoryentry_(			\
    (visBase::FactPtr) clss::createInternal, #clss)




#endif

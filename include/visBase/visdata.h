#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdata.h,v 1.17 2002-05-02 14:15:44 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase { class DataObject; }
typedef visBase::DataObject* (*FactPtr)(void);

namespace visBase
{
class SelectionManager;
class DataManager;

/*!\brief
DataObject is the base class off all objects that are used in Visualization and
ought to be shared in visBase::DataManager. The Data Manager owns all the
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

    virtual SoNode*		getData() { return 0; }

    void			ref() const;
    void			unRef() const;

    virtual bool		selectable() const { return false; }
    void			select() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			deSelect() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    bool			isSelected() const;
    virtual NotifierAccess*	selection() { return 0; }
    virtual NotifierAccess*	deSelection() { return 0; }

    virtual int			usePar( const IOPar& );
    				/*!< Returns -1 on error and 1 on success.
				     If it returns 0 it is missing something.
				     Parse everything else and retry later.
				*/

    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
			
    static const char*		typestr;
    static const char*		namestr;

protected:
    friend			SelectionManager;
    virtual void		triggerSel() {}
    virtual void		triggerDeSel() {}
    
				DataObject();
    virtual			~DataObject();
    void			init();

    friend			DataManager;
    void			remove() { delete this; }
    				/* Should only be called by DataManager */
private:
    int				id_;
    BufferString*		name_;

};



class FactoryEntry
{
public:
    			FactoryEntry( FactPtr, const char*);

    DataObject*		create(void) { return (*funcptr)(); }

    			FactPtr funcptr;
    const char*		name;
};


class Factory
{
public:
    DataObject*			create( const char* );
    ObjectSet<FactoryEntry>	entries;
};

};

#define _mCreateDataObj(clss,args) 				\
{								\
    clss* res = new clss args;					\
    res->init();						\
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

#define _mDeclConstr1Arg(clss,typ,arg)	\
    clss(typ arg);			\
public:

#define _mDeclConstr2Args(clss,typ,arg,typ2,arg2)	\
    clss(typ arg,typ2 arg2);			\
public:

#define _mDeclConstr3Args(clss,typ,arg,typ2,arg2,typ3,arg3)	\
    clss(typ arg,typ2 arg2, typ3 arg3 );			\
public:

#define mCreateDataObj0arg(clss) \
    _mCreateDataObj(clss,()) \
    _mDeclConstr(clss)

#define mCreateDataObj1arg(clss,arg1t,arg1)	\
    _mCreateDataObj(clss,(arg1))		\
    _mDeclConstr1Arg(clss,arg1t,arg1)

#define mCreateDataObj2arg(clss,typ1,arg1,typ2,arg2)	\
    _mCreateDataObj(clss,(arg1,arg2))			\
    _mDeclConstr2Args(clss,typ1,arg1,typ2,arg2)

#define mCreateDataObj3arg(clss,typ1,arg1,typ2,arg2,typ3,arg3)	\
    _mCreateDataObj(clss,(arg1,arg2,arg3))			\
    _mDeclConstr3Args(clss,typ1,arg1,typ2,arg2,typ3,arg3)

#define mCreateFactoryEntry( clss )				\
    visBase::FactoryEntry clss::__factoryentry(			\
	    (FactPtr) clss::create, #clss)

#endif

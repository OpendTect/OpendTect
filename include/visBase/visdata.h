#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdata.h,v 1.5 2002-03-18 14:45:35 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase
{
class SelectionManager;

/*!\brief
DataObject is the base class off all objects that are used in Visualization and
ought to be shared in visBase::DataManager. The Data Manager owns all the
objects and is thus the only one that is allowed to delete it. The destructors
on the inherited classes should thus be protected.
*/

class DataObject : public CallBackClass
{
public:

    int			id() { return id_; }
    const char*		name() const;
    void		setName( const char* );

    void		remove() { delete this; }
    			/* Should only be called by DataManager */
    virtual SoNode*	getData() { return 0; }
    			/*!< May return null if object isn't OpenInventor */

    void		ref() const;
    void		unRef() const;

    virtual i_Notifier*	selection() { return 0; }
    virtual i_Notifier*	deSelection() { return 0; }

    virtual int		usePar( const IOPar& ) { return 1; }
    			/*!< Returns -1 on error and 1 on success.
			     If it returns 0 it is missing something. Parse
			     everything else and retry later.
			*/

    virtual void	fillPar( IOPar& ) const { return; }
			
protected:
    friend			SelectionManager;
    virtual const SoNode*	getSelObj() const { return 0; }
    virtual void		triggerSel() {}
    virtual void		triggerDeSel() {}
    
			DataObject();
    virtual		~DataObject();
    void		init();

private:
    int			id_;
    BufferString*	name_;
};

};

#define mCreateDataObj0arg(clss) 	\
{					\
    clss* res = new clss;		\
    res->init();			\
    return res;				\
}					


#define mCreateDataObj1arg(clss, arg1 ) \
{					\
    clss* res = new clss( arg1 );	\
    res->init();			\
    return res;				\
}

   
#define mCreateDataObj2arg(clss, arg1, arg2 ) \
{					\
    clss* res = new clss( arg1, arg2 );	\
    res->init();			\
    return res;				\
}

   


#endif

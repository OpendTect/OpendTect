
#ifndef visvw2ddata_h
#define visvw2ddata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddata.h,v 1.5 2012-08-03 13:01:17 cvskris Exp $
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "callback.h"
#include "refcount.h"

#include "visvw2ddataman.h"

class IOPar;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

mClass(uiViewer2D) Vw2DDataObject : public CallBacker
{ mRefCountImpl(Vw2DDataObject)
public:

    virtual const char*         getClassName() const    { return "Not impl"; }

    int				id() const		{ return id_; }
    void			setID(int nid)		{ id_ = nid; }

    const char*			name() const;
    virtual void		setName(const char*);

    virtual NotifierAccess*	deSelection()		{ return 0; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyMID()  		{ return "ID"; }

protected:
    				Vw2DDataObject();

    virtual void	triggerDeSel()			{}

    int			id_;
    BufferString*	name_;

    friend class	Vw2DDataManager;
};


mClass(uiViewer2D) Vw2DEMDataObject : public Vw2DDataObject
{ 
public:

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    const EM::ObjectID& emID() const                    { return emid_; }

protected:
    			Vw2DEMDataObject(const EM::ObjectID&,uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);

    uiFlatViewWin*	viewerwin_;
    EM::ObjectID 	emid_;
    virtual void	setEditors()	= 0;

    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};


#define _mCreateVw2DDataObj(clss,id,win,editors)                    \
{                                                               \
    return (clss*) createInternal(id,win,editors);         	\
}                                                               \
								\
private:                                                        \
    static Vw2DDataObject* createInternal(const EM::ObjectID&,	\
	    uiFlatViewWin*,const ObjectSet<uiFlatViewAuxDataEditor>&); \
    clss(const clss&);                                          \
    clss& operator =(const clss&);                              \
public:                                                         \
    static void         initClass();				\
    static const char*  getStaticClassName();                   \
								\
    virtual const char* getClassName() const;                   \
protected:

#define _mDeclVw2DConstr(clss,oid,win,ed)                       \
    clss(const EM::ObjectID& oid,uiFlatViewWin* win,		\
	    const ObjectSet<uiFlatViewAuxDataEditor>& ed);      \
public:

#define mCreateVw2DDataObj(clss,oid,win,ed)                          \
    _mCreateVw2DDataObj(clss,oid,win,ed)                             \
    _mDeclVw2DConstr(clss,oid,win,ed)


#define mImplVisVwr2DInitClass( clss) 				\
void clss::initClass()						\
{                                                               \
    Vw2DDataManager::factory().addCreator(                 	\
	clss::createInternal, #clss );              		\
}

#define mCreateVw2DFactoryEntryNoInitClass( clss )                  \
const char* clss::getStaticClassName() { return #clss; }        \
const char* clss::getClassName() const                          \
{ return clss::getStaticClassName(); }                          \
Vw2DDataObject* clss::createInternal(const EM::ObjectID& oid,	\
	uiFlatViewWin* win,const ObjectSet<uiFlatViewAuxDataEditor>& eds) \
{                                                               \
    return new clss(oid,win,eds);                           	\
}                                                       


#define mCreateVw2DFactoryEntry( clss)  		             	\
mImplVisVwr2DInitClass( clss ); 		  			\
mCreateVw2DFactoryEntryNoInitClass( clss );


#endif


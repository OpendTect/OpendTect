#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddataman.h"
#include "callback.h"
#include "dbkey.h"
#include "geomid.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

mExpClass(uiViewer2D) Vw2DDataObject : public RefCount::Referenced
				     , public CallBacker
{
public:

    virtual const char*         getClassName() const    { return "Not impl"; }

    int				id() const		{ return id_; }
    void			setID(int nid)		{ id_ = nid; }

    const char*			name() const;
    virtual void		setName(const char*);

    virtual NotifierAccess*	deSelection()		{ return 0; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyMID()		{ return "ID"; }

    virtual void		draw()			{}

protected:
			~Vw2DDataObject();
			Vw2DDataObject();

    virtual void	triggerDeSel()			{}

    int			id_;
    BufferString*	name_;

    friend class	Vw2DDataManager;
};


mExpClass(uiViewer2D) Vw2DEMDataObject : public Vw2DDataObject
{
public:

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    const DBKey& emID() const                    { return emid_; }

protected:
			Vw2DEMDataObject(const DBKey&,uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);

    uiFlatViewWin*	viewerwin_;
    DBKey		emid_;
    virtual void	setEditors()	= 0;

    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};


#define _mCreateVw2DDataObj(clss,id,win,editors)                    \
{                                                               \
    return (clss*) createInternal(id,win,editors);	\
}                                                               \
								\
private:                                                        \
    static Vw2DDataObject* createInternal(const DBKey&,	\
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
    clss(const DBKey& oid,uiFlatViewWin* win,		\
	    const ObjectSet<uiFlatViewAuxDataEditor>& ed);      \
public:

#define mCreateVw2DDataObj(clss,oid,win,ed)                          \
    _mCreateVw2DDataObj(clss,oid,win,ed)                             \
    _mDeclVw2DConstr(clss,oid,win,ed)


#define mImplVisVwr2DInitClass( clss)				\
void clss::initClass()						\
{                                                               \
    Vw2DDataManager::factory().addCreator(	\
	clss::createInternal, #clss );		\
}

#define mCreateVw2DFactoryEntryNoInitClass( clss )                  \
const char* clss::getStaticClassName() { return #clss; }        \
const char* clss::getClassName() const                          \
{ return clss::getStaticClassName(); }                          \
Vw2DDataObject* clss::createInternal(const DBKey& oid,	\
	uiFlatViewWin* win,const ObjectSet<uiFlatViewAuxDataEditor>& eds) \
{                                                               \
    return new clss(oid,win,eds);	\
}


#define mCreateVw2DFactoryEntry( clss)			\
mImplVisVwr2DInitClass( clss );					\
mCreateVw2DFactoryEntryNoInitClass( clss );

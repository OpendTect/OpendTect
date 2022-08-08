#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "sharedobject.h"

#include "view2ddataman.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

mExpClass(uiViewer2D) Vw2DDataObject : public SharedObject
{
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    int				id() const		{ return id_; }
    void			setID(int nid)		{ id_ = nid; }

    virtual NotifierAccess*	deSelection()		{ return 0; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyMID()		{ return "ID"; }

protected:
				Vw2DDataObject();
    virtual			~Vw2DDataObject();

    virtual void		triggerDeSel()			{}

    int				id_;

    friend class		Vw2DDataManager;
};


mExpClass(uiViewer2D) Vw2DEMDataObject : public Vw2DDataObject
{
public:

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		setEMObjectID(const EM::ObjectID&);
    const EM::ObjectID& getEMObjectID() const		{ return emid_; }

    mDeprecated("Use getEMObjectID()")
    const EM::ObjectID& emID() const		{ return getEMObjectID(); }

protected:
			Vw2DEMDataObject(uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);

    uiFlatViewWin*	viewerwin_;
    EM::ObjectID	emid_;
    virtual void	setEditors()	= 0;

    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};


#define mDefStd(clss) \
public: \
static void initClass(); \
static clss* create(uiFlatViewWin*,\
		    const ObjectSet<uiFlatViewAuxDataEditor>&); \
~clss(); \
protected: \
clss(uiFlatViewWin*,const ObjectSet<uiFlatViewAuxDataEditor>&); \
private: \
static Vw2DDataObject* createInternal(uiFlatViewWin*, \
				const ObjectSet<uiFlatViewAuxDataEditor>&);

#define mImplStd(clss) \
void clss::initClass() \
{ \
    Vw2DDataManager::factory().addCreator( clss::createInternal, #clss ); \
} \
\
clss* clss::create( uiFlatViewWin* fvw, \
		    const ObjectSet<uiFlatViewAuxDataEditor>& eds ) \
{ \
    return sCast(clss*,createInternal(fvw,eds)); \
} \
\
Vw2DDataObject* clss::createInternal( \
	uiFlatViewWin* fvw, const ObjectSet<uiFlatViewAuxDataEditor>& eds ) \
{ \
    return new clss( fvw, eds ); \
}

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

namespace View2D
{

mExpClass(uiViewer2D) DataObject : public SharedObject
{
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    Vis2DID			id() const		{ return id_; }
    void			setID( Vis2DID id )	{ id_ = id; }

    virtual NotifierAccess*	deSelection()		{ return 0; }

    virtual bool		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyMID()		{ return "ID"; }

protected:
				DataObject();
    virtual			~DataObject();

    virtual void		triggerDeSel()			{}

    Vis2DID			id_;

    friend class		DataManager;
};


mExpClass(uiViewer2D) EMDataObject : public DataObject
{
public:

    bool		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

    void		setEMObjectID(const EM::ObjectID&);
    const EM::ObjectID& getEMObjectID() const		{ return emid_; }

    mDeprecated("Use getEMObjectID()")
    const EM::ObjectID& emID() const		{ return getEMObjectID(); }

protected:
			EMDataObject(uiFlatViewWin*,
				     const ObjectSet<uiFlatViewAuxDataEditor>&);

    uiFlatViewWin*	viewerwin_;
    EM::ObjectID	emid_;
    virtual void	setEditors()	= 0;

    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeditors_;
};

} // namespace View2D


#define mDefStd(clss) \
public: \
static void initClass(); \
static clss* create(uiFlatViewWin*,\
		    const ObjectSet<uiFlatViewAuxDataEditor>&); \
~clss(); \
protected: \
clss(uiFlatViewWin*,const ObjectSet<uiFlatViewAuxDataEditor>&); \
private: \
static DataObject* createInternal(uiFlatViewWin*, \
				const ObjectSet<uiFlatViewAuxDataEditor>&);

#define mImplStd(clss) \
void clss::initClass() \
{ \
    DataManager::factory().addCreator( clss::createInternal, #clss ); \
} \
\
clss* clss::create( uiFlatViewWin* fvw, \
		    const ObjectSet<uiFlatViewAuxDataEditor>& eds ) \
{ \
    return sCast(clss*,createInternal(fvw,eds)); \
} \
\
DataObject* clss::createInternal( \
	uiFlatViewWin* fvw, const ObjectSet<uiFlatViewAuxDataEditor>& eds ) \
{ \
    return new clss( fvw, eds ); \
}

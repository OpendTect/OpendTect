#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "sharedobject.h"

#include "bufstring.h"
#include "coord.h"
#include "draw.h"
#include "emposid.h"
#include "multiid.h"
#include "notify.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class TrcKeyZSampling;
class Executor;
class IOObj;
class IOObjContext;
class TaskRunner;

namespace Geometry { class Element; }
namespace ZDomain { class Def; }

template <class T> class Selector;
template <class T> class Array2D;

namespace EM
{
class EMManager;

/*!
\brief EM object callback data.
*/

mExpClass(EarthModel) EMObjectCallbackData
{
public:
		EMObjectCallbackData()
		    : event( EMObjectCallbackData::Undef )
		{}


		EMObjectCallbackData( const EMObjectCallbackData& data )
		    : event( data.event )
		    , pid0( data.pid0 )
		    , pid1( data.pid1 )
		    , attrib( data.attrib )
		    , flagfor2dviewer( data.flagfor2dviewer )
		{}


    enum Event { Undef, PositionChange, PosIDChange, PrefColorChange, Removal,
		 AttribChange, SectionChange, NameChange, SelectionChange,
		 LockChange, BurstAlert, LockColorChange } event;

    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib = -1; //Used only with AttribChange
    bool	flagfor2dviewer = false;//Used only with BurstAlert for 2DViewer
};



/*!
\brief Thread safe set of EMObjectCallbackData
*/

mExpClass(EarthModel) CBDataSet
{
public:
void addCallBackData( EM::EMObjectCallbackData* data )
{
    Threads::Locker locker( lock_ );
    emcallbackdata_ += data;
}

EM::EMObjectCallbackData* getCallBackData( int idx )
{
    Threads::Locker locker( lock_ );
    return emcallbackdata_.validIdx(idx) ? emcallbackdata_[idx] : 0;
}

void clearData()
{
    Threads::Locker locker( lock_ );
    deepErase( emcallbackdata_ );
}

int size() const
{
    return emcallbackdata_.size();
}

protected:
    ObjectSet<EMObjectCallbackData>	emcallbackdata_;
    Threads::Lock			lock_;
};



/*!
\brief Iterator that iterates a number of positions (normally all) on an
EMObject. The object is created by EMObject::createIterator, and the next()
function is called until no more positions can be found.
*/

mExpClass(EarthModel) EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next()		= 0;
			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		approximateSize() const { return maximumSize(); }
    virtual int		maximumSize() const	{ return -1; }
    virtual bool	canGoTo() const		{ return false; }
    virtual EM::PosID	goTo(od_int64)		{ return EM::PosID(); }
};


/*!
\brief Position attribute
*/

mExpClass(EarthModel) PosAttrib
{
public:

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode,
			  IntersectionNode };

			PosAttrib(Type);

    Type		type_;
    TypeSet<PosID>	posids_;
    bool		locked_ = false;
};


/*!
\brief Base class for all EarthModel objects.
*/

mExpClass(EarthModel) EMObject : public SharedObject
{
public:
    enum NodeSourceType		{ None = (int)'0', Manual=(int)'1',
				  Auto=(int)'2', Gridding=(int)'3' };

    const ObjectID&		id() const		{ return id_; }
    virtual const char*		getTypeStr() const			= 0;
    virtual uiString		getUserTypeStr() const			= 0;
    const MultiID&		multiID() const		{ return storageid_; }
    void			setMultiID(const MultiID&);

    virtual bool		isOK() const		{ return true; }
    mDeprecatedDef uiString	uiName() const { return toUiString(name()); }
    virtual void		setNewName();

    int				nrSections() const	{ return 1; }
    SectionID			sectionID(int) const
				{ return SectionID::def(); }
    BufferString		sectionName(const SectionID&) const;
    bool			canSetSectionName() const;
    bool			setSectionName(const SectionID&,const char*,
					       bool addtohistory);
    int				sectionIndex(const SectionID&) const;
    bool			removeSection(SectionID,bool hist )
				{ return false; }

    const Geometry::Element*	geometryElement() const;
    Geometry::Element*		geometryElement();

    const OD::Color&		preferredColor() const;
    void			setPreferredColor(const OD::Color&,
						  bool addtohistory=false);
    const OD::LineStyle&		preferredLineStyle() const;
    void			setPreferredLineStyle(const OD::LineStyle&);
    void			setBurstAlert(bool yn);
    bool			hasBurstAlert() const;

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual Coord3		getPos(const EM::SubID&) const;
    virtual bool		isDefined(const EM::PosID&) const;
    virtual bool		isDefined(const EM::SubID&) const;
    virtual bool		setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory);
    virtual bool		setPos(const EM::SubID&,
				       const Coord3&,bool addtohistory);
    virtual bool		unSetPos(const EM::PosID&,bool addtohistory);
    virtual bool		unSetPos(const EM::SubID&,bool addtohistory);
    virtual void		setNodeSourceType(const TrcKey&,
						  NodeSourceType){};
    virtual bool		isNodeSourceType(const PosID&,
				    NodeSourceType) const {return false;}
    virtual bool		isNodeSourceType(const TrcKey&,
				     NodeSourceType)const {return false;}

    virtual void		setNodeLocked(const TrcKey&,bool locked){};
    virtual bool		isNodeLocked(const TrcKey&) const
					    { return false; }
    virtual bool		isNodeLocked(const PosID&)const {return false;}

    virtual void		lockAll() {};
    virtual void		unlockAll(){};
    virtual const Array2D<char>*
				getLockedNodes() const { return 0; }
    virtual void		setLockColor(const OD::Color&);
    virtual const OD::Color&	getLockColor() const;
    virtual bool		hasLockedNodes() const {return haslockednodes_;}
    virtual bool		hasNodeSourceType( const PosID& ) const
						   { return false; }

    void			setSelectionColor(const OD::Color&);
    const OD::Color&		getSelectionColor() const;

    virtual bool		enableGeometryChecks(bool);
    virtual bool		isGeometryChecksEnabled() const;

    virtual bool		isAtEdge(const EM::PosID&) const;

    void			changePosID(const EM::PosID& from,
					    const EM::PosID& to,
					    bool addtohistory);
				/*!<Tells the object that the node former known
				    as from is now called to. Function will also
				    exchange set the position of to to the
				    posion of from. */

    virtual void		getLinkedPos(const EM::PosID& posid,
					  TypeSet<EM::PosID>&) const
					{ return; }
				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual EMObjectIterator*	createIterator(const TrcKeyZSampling*) const
				{ return nullptr; }

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual bool		hasPosAttrib(int attr) const;
    virtual void		addPosAttrib(int attr);
    virtual void		removePosAttribList(int attr,
						    bool addtohistory=true);
    virtual void		setPosAttrib(const EM::PosID&,
				    int attr,bool yn,bool addtohistory=true);
				//!<Sets/unsets the posattrib depending on yn.
    virtual bool		isPosAttrib(const EM::PosID&,int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int attr) const;
    const MarkerStyle3D&	getPosAttrMarkerStyle(int attr) const;
    void			setPosAttrMarkerStyle(int attr,
						      const MarkerStyle3D&);
    virtual void		lockPosAttrib(int attr,bool yn);
    virtual bool		isPosAttribLocked(int attr) const;
    virtual void		removeSelected(const Selector<Coord3>&,
					       TaskRunner*);
    void			removeSelected(const TypeSet<EM::SubID>&);

    void			removeListOfSubIDs(const TypeSet<EM::SubID>&);
    void			removeAllUnSeedPos();
    const TrcKeyZSampling		getRemovedPolySelectedPosBox();
    void			emptyRemovedPolySelectedPosBox();

    CNotifier<EMObject,const EMObjectCallbackData&>	change;

    virtual Executor*		loader()		{ return 0; }
    virtual bool		isLoaded() const	{ return false; }
    virtual Executor*		saver()			{ return 0; }
    virtual bool		isChanged() const	{ return changed_; }
    virtual bool		isEmpty() const;
    virtual void		setChangedFlag()	{ changed_=true; }
    virtual void		resetChangedFlag()	{ changed_=false; }
    bool			isFullyLoaded() const	{ return fullyloaded_; }
    void			setFullyLoaded(bool yn) { fullyloaded_=yn; }

    virtual bool		isLocked() const	{ return locked_; }
    virtual void		lock(bool yn)		{ locked_=yn;}

    bool			isInsideSelRemoval() const
				{ return insideselremoval_; }
    bool			isSelRemoving() const	{ return selremoving_; }

    uiString			errMsg() const;
    void			setErrMsg(const uiString& m) { errmsg_ = m; }

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    void			saveDisplayPars() const;

    virtual bool		useDisplayPar(const IOPar&);
    virtual void		fillDisplayPar(IOPar&) const;

    static int			sPermanentControlNode();
    static int			sTemporaryControlNode();
    static int			sEdgeControlNode();
    static int			sTerminationNode();
    static int			sSeedNode();
    static int			sIntersectionNode();

    virtual const IOObjContext& getIOObjContext() const = 0;

    Interval<float>		getZRange(bool compute_if_needed=false) const;
    bool			isZInDepth() const;
    void			setZInDepth();
    void			setZInTime();

// Deprecated public functions
    mDeprecated("Use geometryElement() const")
    const Geometry::Element*	sectionGeometry(const SectionID&) const;
    mDeprecated("Use geometryElement()")
    Geometry::Element*		sectionGeometry(const SectionID&);

    mDeprecated("Use removeListOfSubIDs without SectionID")
    void			removeListOfSubIDs(const TypeSet<EM::SubID>&,
						   const EM::SectionID&);
    mDeprecated("Use createIterator(const TrcKeyZSampling*)")
    EMObjectIterator*		createIterator(const EM::SectionID&,
					       const TrcKeyZSampling* t=0) const
				{ return createIterator(t); }
				/*!< creates an iterator. If the sectionid is
				     -1, all sections will be traversed. */

    mDeprecated("Use without SectionID")
    Coord3			getPos(const EM::SectionID&,
				       const EM::SubID& subid) const
				{ return getPos(subid); }
    mDeprecated("Use without SectionID")
    bool			isDefined(const EM::SectionID&,
					  const EM::SubID& subid) const
				{ return isDefined(subid); }
    mDeprecated("Use without SectionID")
    bool			setPos(const EM::SectionID&,
					const EM::SubID& subid,
					const Coord3& crd,bool addtohistory)
				{ return setPos(subid,crd,addtohistory); }
    mDeprecated("Use without SectionID")
    bool			unSetPos(const EM::SectionID&,
					const EM::SubID& subid,
					bool addtohistory)
				{ return unSetPos(subid,addtohistory); }

protected:
				~EMObject();
				EMObject( EMManager& );
				//!<must be called after creation
    virtual Geometry::Element*	geometryElementInternal() { return nullptr; }

    virtual void		prepareForDelete() const;
    void			prepareForDelete() override;
    void			posIDChangeCB(CallBacker*);
    const MarkerStyle3D&	preferredMarkerStyle3D() const;
    void			setPreferredMarkerStyle3D(const MarkerStyle3D&);
    void			useDisplayPars(const IOPar&);

    BufferString		objname_;
    ObjectID			id_;
    MultiID			storageid_;
    class EMManager&		manager_;
    uiString			errmsg_;

    OD::Color&			preferredcolor_;
    OD::LineStyle&		preferredlinestyle_;
    MarkerStyle3D&		preferredmarkerstyle_;
    ObjectSet<PosAttrib>	posattribs_;
    TypeSet<int>		attribs_;
    MarkerStyle3D&		posattribmarkerstyle_;

    TrcKeyZSampling		removebypolyposbox_;

    bool			changed_ = false;
    bool			fullyloaded_ = false;
    bool			locked_ = false;
    int				burstalertcount_ = 0;
    Threads::Lock		setposlock_;
    bool			haslockednodes_ = false;
    OD::Color			lockcolor_;
    OD::Color			selectioncolor_;

    bool			insideselremoval_ = false;
    bool			selremoving_ = false;
    const ZDomain::Def*		zdomain_;

    static const char*		nrposattrstr();
    static const char*		posattrprefixstr();
    static const char*		posattrsectionstr();
    static const char*		posattrposidstr();

// Deprecated protected functions
    mDeprecated("Use geometryElementInternal()")
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&)
				{ return nullptr; }
};

} // namespace EM

#define mDefineEMObjFuncs( clss ) \
mODTextTranslationClass( clss ); \
public: \
				clss(EM::EMManager&); \
    static void			initClass(); \
    static EMObject*		create(EM::EMManager&); \
    static clss*		create(const char* nm); \
    static StringView		typeStr(); \
    const char*			getTypeStr() const override; \
    void			setNewName() override; \
protected: \
				~clss()

#define mImplementEMObjFuncs( clss, typenm ) \
void clss::initClass() \
{ \
    EMOF().addCreator( create, typeStr() ); \
} \
 \
 \
EMObject* clss::create( EM::EMManager& emm ) \
{ \
    EMObject* obj = new clss( emm ); \
    if ( !obj ) return 0; \
    obj->ref();		\
    emm.addObject( obj ); \
    obj->unRefNoDelete(); \
    return obj; \
} \
\
clss* clss::create( const char* nm ) \
{ \
    const ObjectID objid = EMM().createObject( typeStr(), nm ); \
    EMObject* emobj = EMM().getObject( objid ); \
    mDynamicCastGet(clss*,newobj,emobj); \
    return newobj; \
} \
\
StringView clss::typeStr() { return typenm; } \
const char* clss::getTypeStr() const { return typeStr(); } \
void clss::setNewName() \
{\
    static int objnr = 1; \
    BufferString nm( "<New ", typenm, " " ); \
    nm.add( objnr++ ).add( ">" ); \
    setName( nm ); \
}


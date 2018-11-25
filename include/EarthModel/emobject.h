#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emcommon.h"

#include "notify.h"
#include "trckeyzsampling.h"
#include "draw.h"
#include "dbkey.h"
#include "coord.h"
#include "sharedobject.h"
#include "uistring.h"
#include "taskrunner.h"

class TrcKeyZSampling;
class Executor;
class IOObj;
class IOObjContext;

namespace Geometry { class Element; }

template <class T> class Selector;
template <class T> class Array2D;

namespace EM
{
class ObjectManager;

/*!\brief EM object callback data.  */

typedef Monitorable::ChangeData ObjectCallbackData;

mExpClass(EarthModel) ChangeAuxData : public Monitorable::ChangeData::AuxData
{
public:
		ChangeAuxData()
		    : attrib( -1 )
		    , flagfor2dviewer( false )
		{}


		ChangeAuxData( const ChangeAuxData& data )
		    : pid0( data.pid0 )
		    , pid1( data.pid1 )
		    , attrib( data.attrib )
		    , flagfor2dviewer( data.flagfor2dviewer )
		{}


    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib; //Used only with AttribChange
    bool	flagfor2dviewer; //Used only with BurstAlert for 2DViewer

};



/*!\brief Thread safe set of ObjectCallbackData */

mExpClass(EarthModel) CBDataSet
{
public:
void addCallBackData( const EM::ObjectCallbackData* data )
{
    Threads::Locker locker( lock_ );
    emcallbackdata_ += data;
}

const EM::ObjectCallbackData* getCallBackData( int idx )
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
    ObjectSet<const ObjectCallbackData>	emcallbackdata_;
    Threads::Lock			lock_;
};



/*!
\brief Iterator that iterates a number of positions (normally all) on an
EM::Object. The object is created by EM::Object::createIterator, and the next()
function is called until no more positions can be found.
*/

mExpClass(EarthModel) ObjectIterator
{
public:

    virtual		~ObjectIterator() {}

    virtual EM::PosID	next()		= 0;
			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		approximateSize() const	{ return maximumSize(); }
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
			PosAttrib(){}

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode,
			  IntersectionNode };

    Type		type_		= PermanentControlNode;
    TypeSet<PosID>	posids_;
    bool		locked_		= false;
};


/*!
\brief Base class for all EarthModel objects.
*/

#define mImplEMSet(fnnm,typ,memb,emcbtype) \
void fnnm( typ _set_to_ ) \
{ setMemberSimple( memb, _set_to_, emcbtype, ChangeData::cUnspecChgID() ); }

#define mImplEMGetSet(pfx,fnnmget,fnnmset,typ,memb,emcbtype) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplEMSet(fnnmset,const typ&,memb,emcbtype)


mExpClass(EarthModel) Object : public SharedObject
{
public:

    enum NodeSourceType		{ None = (int)'0', Manual=(int)'1',
				  Auto=(int)'2', Gridding=(int)'3' };

    const DBKey&		id() const		{ return storageid_; }
    virtual const char*		getTypeStr() const	= 0;
    virtual uiString		getUserTypeStr() const	= 0;
    const DBKey&		dbKey() const		{ return storageid_; }
    void			setDBKey(const DBKey&);

    mImplEMGetSet(inline,preferredColor,setPreferredColor,Color,preferredcolor_,
		  cPrefColorChange())
    mImplEMGetSet(inline,selectionColor,setSelectionColor,Color,selectioncolor_,
		  cSelColorChange())
    mImplEMGetSet(inline,preferredLineStyle,setPreferredLineStyle,
		  OD::LineStyle, preferredlinestyle_,cPrefLineStyleChange())
    mImplEMGetSet(inline,preferredMarkerStyle3D,setPreferredMarkerStyle3D,
		  OD::MarkerStyle3D,preferredmarkerstyle_,
		  cPrefMarkerStyleChange())

    virtual bool		isOK() const		{ return true; }
    virtual void		setNameToJustCreated();

    virtual const Geometry::Element*	geometryElement() const;
    virtual Geometry::Element*		geometryElement();

    void			setBurstAlert(bool yn);
    bool			hasBurstAlert() const;

    virtual Coord3		getPos(const EM::PosID&) const;

    virtual bool		isDefined(const EM::PosID&) const;

    bool			setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory,
				       NodeSourceType type=Auto);

    virtual bool		unSetPos(const EM::PosID&,bool addtohistory);

    virtual void		setNodeSourceType(const TrcKey&,
							NodeSourceType){}
    virtual bool		hasNodeSourceType( const PosID& ) const
						    { return false; }

    virtual bool		isNodeSourceType(const PosID&,
				    NodeSourceType) const {return false;}
    virtual bool		isNodeSourceType(const TrcKey&,
				     NodeSourceType)const {return false;}

    virtual void		setNodeLocked(const TrcKey&,bool locked){}
    virtual bool		isNodeLocked(const TrcKey&) const
					    { return false; }
    virtual bool		isNodeLocked(const PosID&)const {return false;}

    virtual void		lockAll() {}
    virtual void		unlockAll(){}
    virtual const Array2D<char>*
				getLockedNodes() const { return 0; }
    virtual void		setLockColor(const Color&);
    virtual const Color		getLockColor() const;
    virtual bool		hasLockedNodes() const {return haslockednodes_;}


    virtual bool		enableGeometryChecks(bool);
    virtual bool		isGeometryChecksEnabled() const;

    virtual bool		isAtEdge(const EM::PosID&) const;


    virtual void		getLinkedPos(const EM::PosID& posid,
					  TypeSet<EM::PosID>&) const
					{ return; }
				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual ObjectIterator*	createIterator(const TrcKeyZSampling* =0) const
				{ return 0; }

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
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
    OD::MarkerStyle3D		getPosAttrMarkerStyle(int attr);
    void			setPosAttrMarkerStyle(int attr,
						      const OD::MarkerStyle3D&);
    virtual void		lockPosAttrib(int attr,bool yn);
    virtual bool		isPosAttribLocked(int attr) const;
    virtual void		removeSelected(const Selector<Coord3>&,
					       const TaskRunnerProvider&);
    void			removePositions(const TypeSet<EM::PosID>&);
    void			removeAllUnSeedPos();
    const TrcKeyZSampling	getRemovedPolySelectedPosBox();
    void			emptyRemovedPolySelectedPosBox();

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

    static int			sTerminationNode();
    static int			sSeedNode();
    static int			sIntersectionNode();

    static ChangeType		cUndefChange()
				{ return ChangeData::cNoChgType(); }
    static ChangeType		cPositionChange()	{ return 2; }
    static ChangeType		cPrefColorChange()	{ return 3; }
    static ChangeType		cSelColorChange()	{ return 4; }
    static ChangeType		cPrefLineStyleChange()	{ return 5; }
    static ChangeType		cPrefMarkerStyleChange(){ return 6; }
    static ChangeType		cPosIDChange()		{ return 7; }
    static ChangeType		cAttribChange()		{ return 8; }
    static ChangeType		cNameChange()		{ return 10; }
    static ChangeType		cSelectionChange()	{ return 11; }
    static ChangeType		cLockChange()		{ return 12; }
    static ChangeType		cBurstAlert()		{ return 13; }
    static ChangeType		cLockColorChange()	{ return 14; }
    static ChangeType		cParentColorChange()	{ return 15; }
    static ChangeType		cSeedChange()		{ return 16; }
    static ChangeType		cObjectRemoved()	{ return 17; }

    virtual const IOObjContext&	getIOObjContext() const = 0;


protected:

				~Object();
				Object(const char*);
				//!<must be called after creation

				mDeclAbstractMonitorableAssignment(Object);

    virtual bool		setPosition(const EM::PosID&,
					    const Coord3&,bool addtohistory,
					    NodeSourceType type=Auto);
    virtual void		prepareForDelete();
    void			posIDChangeCB(CallBacker*);
    void			useDisplayPars(const IOPar&);

    BufferString		objname_;
    DBKey			storageid_;
    uiString			errmsg_;

    Color			preferredcolor_;
    Color			lockcolor_;
    Color			selectioncolor_;
    OD::LineStyle		preferredlinestyle_;
    OD::MarkerStyle3D		preferredmarkerstyle_;
    ObjectSet<PosAttrib>	posattribs_;
    TypeSet<int>		attribs_;

    TrcKeyZSampling		removebypolyposbox_;

    bool			changed_;
    bool			fullyloaded_;
    bool			locked_;
    int				burstalertcount_;
    Threads::Lock		setposlock_;

    bool			insideselremoval_;
    bool			selremoving_;
    bool			haslockednodes_;

    static const char*		nrposattrstr();
    static const char*		posattrprefixstr();
    static const char*		posattrsectionstr();
    static const char*		posattrposidstr();

public:

    mDeprecated const DBKey&	multiID() const		{ return storageid_; }
    mDeprecated void		setMultiID( const DBKey& k ) { setDBKey(k); }
    static Color		sDefaultSelectionColor();
    static Color		sDefaultLockColor();

				//TODO:Remove when all Objects are Montorable
    void			sendEMNotifFromOutside(
					const ObjectCallbackData& c) const
				{ mLock4Read(); sendChgNotif(accesslocker_,c); }
};

mDeprecated typedef Object	EMObject;

} // namespace EM

#define mDefineEMObjFuncs( clss ) \
public: \
				clss(const char* nm); \
    static void			initClass(); \
    static EM::Object*		create(EM::ObjectManager&); \
    static clss*		create(const char* nm); \
    static FixedString		typeStr(); \
    const char*			getTypeStr() const; \
    void			setNameToJustCreated(); \
protected: \
				~clss()

#define mImplementEMObjFuncs( clss, typenm ) \
void clss::initClass() \
{ \
    EMOF().addCreator( create, typeStr() ); \
} \
 \
 \
EM::Object* clss::create( EM::ObjectManager& emm ) \
{ \
    EM::Object* obj = new clss(""); \
    if ( !obj ) return 0; \
    obj->ref();         \
    obj->unRefNoDelete(); \
    return obj; \
} \
\
clss* clss::create( const char* nm ) \
{ \
    EM::Object* emobj = MGR().createObject( typeStr(), nm ); \
    mDynamicCastGet(clss*,newobj,emobj); \
    return newobj; \
} \
\
FixedString clss::typeStr() { return typenm; } \
const char* clss::getTypeStr() const { return typeStr(); } \
void clss::setNameToJustCreated() \
{\
    static int objnr = 1; \
    BufferString nm( "<New ", typenm, " " ); \
    nm.add( objnr++ ).add( ">" ); \
    setName( nm ); \
}


#define mSendEMCBNotifWithData( typ, data ) \
    EM::ObjectCallbackData cd( typ, Monitorable::ChangeData::cUnspecChgID(), \
			     data ); \
    sendChgNotif( accesslocker_, cd );

#define mSendEMCBNotifPosID( typ, pid ) \
    setChangedFlag(); \
    RefMan<ChangeAuxData> data = new ChangeAuxData; \
    data->pid0 = pid; \
    EM::ObjectCallbackData cd( typ, Monitorable::ChangeData::cUnspecChgID(), \
			     data ); \
    sendChgNotif( accesslocker_, cd );

#define mSendEMCBNotif( typ ) \
    setChangedFlag(); \
    EM::ObjectCallbackData cd(typ,Monitorable::ChangeData::cUnspecChgID(),0); \
    sendChgNotif( accesslocker_, cd );

#define mSendEMSurfNotif( typ ) \
    surface_.setChangedFlag(); \
    EM::ObjectCallbackData cd(typ,Monitorable::ChangeData::cUnspecChgID(),0); \
    surface_.sendEMNotifFromOutside( cd );

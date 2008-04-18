#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.61 2008-04-18 10:49:40 cvsnanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class CubeSampling;
class BinID;

namespace visBase 
{ 
    class SplitTextureRandomLine; 
    class MultiTexture2; 
    class EventCatcher; 
    class RandomTrackDragger; 
};

namespace Attrib { class SelSpec; }

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying a random or arbitrary line.

    RandomTrackDisplay is the front-end class for displaying arbitrary lines.
    The complete line consists of separate sections connected at 
    inline/crossline positions, called knots or nodes. Several functions are
    available for adding or inserting knot positions. The depth range of the
    line can be changed by <code>setDepthInterval(const Interval<float>&)</code>
*/

class RandomTrackDisplay :  public visBase::VisualObjectImpl,
			    public visSurvey::SurveyObject
{
public:
    static RandomTrackDisplay*	create()
				mCreateDataObj(RandomTrackDisplay);
    int				nameNr() const { return namenr_; }
    				/*!<\returns a number that is unique for 
				     this rtd, and is present in its name. */

    bool			isInlCrl() const { return true; }

    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const { return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;

    bool			allowMaterialEdit() const { return true; }

    SurveyObject::AttribFormat	getAttributeFormat() const;
    bool			canHaveMultipleAttribs() const;
    bool			swapAttribs(int,int);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			isAttribEnabled(int attrib) const;
    void			enableAttrib(int attrib,bool yn);
    bool			isAngle(int attrib) const;
    void			setAngleFlag(int attrib,bool yn);
    void			allowShading(bool);

    bool			removeAttrib(int);
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec( int, const Attrib::SelSpec& );

    int				nrTextures(int)const;
    void			selectTexture(int,int);
    int				selectedTexture(int)const;

    void			getDataTraceBids(TypeSet<BinID>&) const;
    Interval<float>		getDataTraceRange() const;
    void			setTraceData(int,SeisTrcBuf&);

    bool                        setDataPackID(int attrib,DataPack::ID);
    DataPack::ID                getDataPackID(int attrib) const;
    virtual DataPackMgr::ID     getDataPackMgrID() const
				{ return DataPackMgr::FlatID; }

    bool			canAddKnot(int knotnr) const;
    				/*!< If knotnr<nrKnots the function Checks if
				     a knot can be added before the
				     knotnr.
				     If knotnr==nrKnots, it checks if a knot
				     can be added.
				*/
    void			addKnot(int knotnr);
    				/*! if knotnr<nrKnots, a knot is added before
				    the knotnr.
				    If knotnr==nrKnots, a knot is added at the
				    end
				*/
    				
    int				nrKnots() const;
    void			addKnot(const BinID&);
    void			insertKnot(int,const BinID&);
    void			setKnotPos(int,const BinID&);
    BinID			getKnotPos(int) const;
    BinID			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<BinID>&) const;
    void			removeKnot(int);
    void			setKnotPositions(const TypeSet<BinID>&);
    void			lockGeometry(bool);
    bool			isGeometryLocked() const;

    void			setDepthInterval(const Interval<float>&);
    Interval<float>		getDepthInterval() const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
	    					BufferString& val,
	    					BufferString& info) const;

    int				getColTabID(int) const;
    const TypeSet<float>*	getHistogram(int) const;

    int				getSelKnotIdx() const	{ return selknotidx_; }

    virtual NotifierAccess*	getMovementNotifier() 	{ return &moving_; }
    NotifierAccess*		getManipulationNotifier() {return &knotmoving_;}

    Coord3			getNormal(const Coord3&) const;
    virtual float               calcDist(const Coord3&) const;
    virtual bool		allowPicks() const		{ return true; }

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

    bool			canBDispOn2DViewer() const	{ return true; }
    TypeSet<BinID>*             getPath()		{ return &trcspath_; }
    
    Notifier<RandomTrackDisplay> moving_;
    Notifier<RandomTrackDisplay> knotmoving_;

protected:
				~RandomTrackDisplay();

    void			getDataTraceBids(TypeSet<BinID>&,
	    					 TypeSet<int>* segments) const;
    BinID			proposeNewPos(int knot) const;
    void			setData( int attrib, const SeisTrcBuf& );

    void			setKnotPos(int,const BinID&,bool check);

    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			knotMoved(CallBacker*);
    void			knotNrChanged(CallBacker*);


    visBase::SplitTextureRandomLine* triangles_;
    visBase::RandomTrackDragger* dragger_;
    visBase::MultiTexture2*	texture_;
    ObjectSet<Attrib::SelSpec>	as_;
    ObjectSet<SeisTrcBuf>	cache_;
    int				selknotidx_;
    TypeSet<DataPack::ID>	datapackids_;
    TypeSet<BinID>		trcspath_;
    TypeSet<BinID>		knots_;

    ZAxisTransform*		datatransform_;

    bool			lockgeometry_;
    bool			ismanip_;
    int				namenr_;

    static const char*		sKeyTrack();
    static const char*		sKeyNrKnots();
    static const char*		sKeyKnotPrefix();
    static const char*		sKeyDepthInterval();
    static const char*		sKeyLockGeometry();

};

};


#endif

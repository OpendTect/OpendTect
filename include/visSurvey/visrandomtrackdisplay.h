#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.35 2004-11-30 08:07:03 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class ColorAttribSel;
class AttribSelSpec;
class CubeSampling;
class BinID;

namespace visBase { class RandomTrack; class Material; 
		    class EventCatcher; };

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

class RandomTrackDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:
    static RandomTrackDisplay*	create()
				mCreateDataObj(RandomTrackDisplay);

    bool			isInlCrl() const { return true; }

    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const { return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;

    int                		nrResolutions() const; 
    int				getResolution() const;
    void			setResolution(int);

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;

    bool			allowMaterialEdit() const { return true; }

    int				getAttributeFormat() const { return 1; }
    bool			hasColorAttribute() const { return true; }
    const AttribSelSpec*	getSelSpec() const { return &as; }
    const ColorAttribSel*	getColorSelSpec() const { return &colas; }
    void			setSelSpec( const AttribSelSpec& );
    void                        setColorSelSpec(const ColorAttribSel&);

    void			getDataTraceBids(TypeSet<BinID>&) const;
    Interval<float>		getDataTraceRange() const;
    void			setTraceData(bool,SeisTrcBuf&);

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
    void			setManipKnotPos(int,const BinID&);
    BinID			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<BinID>&) const;
    void			removeKnot(int);
    void			removeAllKnots();

    void			setDepthInterval(const Interval<float>&);

    void			getMousePosInfo(const Coord3&,float& val,
	    					BufferString& info) const;

    void			turnOn(bool);
    bool			isOn() const;

    int				getColTabID() const;
    const TypeSet<float>*	getHistogram() const;

    void                        setMaterial( visBase::Material* );
    const visBase::Material*    getMaterial() const;
    visBase::Material*          getMaterial();

    int				getSectionIdx() const;
    void			removeNearestKnot(int,const BinID&);

    int				getSelKnotIdx() const	{ return selknotidx; }

    virtual NotifierAccess*	getMovementNotification() { return &moving; }
    NotifierAccess*		getManipulationNotifier() { return &knotmoving;}

    SoNode*			getInventorNode();

    virtual float               calcDist(const Coord3&) const;
    virtual bool		allowPicks() const		{ return true; }

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~RandomTrackDisplay();

    void			setRandomTrack(visBase::RandomTrack*);

    BinID			proposeNewPos(int knot) const;

    visBase::RandomTrack*	track;
    visBase::Material*		texturematerial;
    AttribSelSpec&		as;
    ColorAttribSel&		colas;
    int				selknotidx;

    void			setData(const SeisTrcBuf&,int datatp=0);

    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			knotMoved(CallBacker*);
    void			knotNrChanged(CallBacker*);

    ObjectSet< TypeSet<BinID> > bidsset;
    SeisTrcBuf&			cache;
    SeisTrcBuf&			colcache;
    bool			ismanip;

    static const char*		trackstr;
    static const char*		nrknotsstr;
    static const char*		knotprefix;
    static const char*		depthintvstr;

    Notifier<RandomTrackDisplay> moving;
    Notifier<RandomTrackDisplay> knotmoving;
};

};


#endif

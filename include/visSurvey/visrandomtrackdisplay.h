#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.25 2004-04-27 12:29:57 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class ColorAttribSel;
class AttribSelSpec;
class CubeSampling;
class BinID;
class SeisTrc;
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

    int                		nrResolutions() const; 
    BufferString		getResolutionName(int) const;
    int				getResolution() const;
    void			setResolution(int);

    bool			hasMaterial() const { return true; }

    int				getAttributeFormat() const { return 1; }
    bool			hasColorAttribute() const { return true; }
    const AttribSelSpec*	getSelSpec() const { return &as; }
    const ColorAttribSel*	getColorSelSpec() const { return &colas; }
    void			setSelSpec( const AttribSelSpec& );
    void                        setColorSelSpec(const ColorAttribSel&);

    void			getDataTraceBids(TypeSet<BinID>&) const;
    Interval<float>		getDataTraceRange() const;
    void			setTraceData( bool color,
	    				      ObjectSet<SeisTrc>* trcs);
    
    int				nrKnots() const;
    void			addKnot(const BinID&);
    void			addKnots(TypeSet<BinID>);
    void			insertKnot(int,const BinID&);
    void			setKnotPos(int,const BinID&);
    BinID			getKnotPos(int) const;
    BinID			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<BinID>&) const;
    void			removeKnot(int);
    void			removeAllKnots();

    float			getValue(const Coord3&) const;

    void			turnOn(bool);
    bool			isOn() const;

    int				getColTabID() const;
    const TypeSet<float>*	getHistogram() const;

    void                        setMaterial( visBase::Material* );
    const visBase::Material*    getMaterial() const;
    visBase::Material*          getMaterial();

    int				getSectionIdx() const;
    BinID			getClickedPos() const;
    void			removeNearestKnot(int,const BinID&);

    Notifier<RandomTrackDisplay> rightclick;
    Notifier<RandomTrackDisplay> knotmoving;
    int				getSelKnotIdx() const	{ return selknotidx; }

    virtual float               calcDist(const Coord3&) const;
    virtual NotifierAccess*	getMovementNotification() { return &moving; }

    SoNode*			getInventorNode();

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~RandomTrackDisplay();
    void			setDepthInterval(const Interval<float>&);

    void			setRandomTrack(visBase::RandomTrack*);

    visBase::RandomTrack*	track;
    visBase::Material*		texturematerial;
    AttribSelSpec&		as;
    ColorAttribSel&		colas;
    int				selknotidx;

    void			setData(const ObjectSet<SeisTrc>&,int datatp=0);
    const SeisTrc*		getTrc(const BinID&,const ObjectSet<SeisTrc>&)
									const;
    void			checkPosition(BinID&);

    void			knotMoved(CallBacker*);
    void			knotNrChanged(CallBacker*);
    void			rightClicked(CallBacker*);

    ObjectSet< TypeSet<BinID> > bidsset;
    ObjectSet<SeisTrc>		cache;
    ObjectSet<SeisTrc>		colcache;
    bool			ismanip;

    static const char*		trackstr;
    static const char*		nrknotsstr;
    static const char*		knotprefix;
    static const char*		depthintvstr;

    Notifier<RandomTrackDisplay>  moving;
};

};


#endif

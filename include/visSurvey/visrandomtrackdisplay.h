#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.13 2003-03-10 15:50:49 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class CubeSampling;
class BinID;
class SeisTrc;
class BinID;

namespace visBase { class RandomTrack; class VisColorTab; class Material; 
		    class EventCatcher; };

namespace visSurvey
{

class Scene;

/*!\brief
    RandomTrackDisplay contains a RandomTrack for displaying seismics or attributes.
*/

class RandomTrackDisplay :  public visBase::VisualObject,
			  public visSurvey::SurveyObject
{
public:

    static RandomTrackDisplay*	create()
				mCreateDataObj(RandomTrackDisplay);

    void			setAttribSelSpec(const AttribSelSpec&);
    AttribSelSpec&		getAttribSelSpec();
    const AttribSelSpec&	getAttribSelSpec() const;

    void			setDepthInterval(const Interval<float>&);
    const Interval<float>	getDepthInterval() const;
    const Interval<float>	getManipDepthInterval() const;
    

    int				nrKnots() const;
    void			addKnot(const BinID&);
    void			addKnots(TypeSet<BinID>);
    void			insertKnot(int,const BinID&);
    void			setKnotPos(int,const BinID&);
    BinID			getKnotPos(int) const;
    BinID			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<BinID>&);
    void			removeKnot(int);
    void			removeAllKnots();
    bool			isManipulated() const;
    void			acceptManip();

    void			getDataPositions(TypeSet<BinID>&);
    bool			putNewData(const ObjectSet<SeisTrc>&);
    float			getValue(const Coord3&) const;

    void			showDragger(bool);
    bool			isDraggerShown() const;
    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTab(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTab();

    void                        setMaterial( visBase::Material* );
    const visBase::Material*    getMaterial() const;
    visBase::Material*          getMaterial();

    void			setResolution(int);
    int				getResolution() const;
    const char*			getResName(int) const;
    int				getNrResolutions() const;

    int				getSectionIdx() const;
    BinID			getClickedPos() const;
    void			removeNearestKnot(int,const BinID&);

    Notifier<RandomTrackDisplay> rightclick;
    Notifier<RandomTrackDisplay> knotmoving;
    int				getSelKnotIdx() const	{ return selknotidx; }

    SoNode*			getData();

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~RandomTrackDisplay();

    visBase::RandomTrack*	track;
    visBase::Material*		texturematerial;
    AttribSelSpec&		as;
    int				selknotidx;

    void			setData(const ObjectSet<SeisTrc>&);
    const SeisTrc*		getTrc(const BinID&,const ObjectSet<SeisTrc>&)
									const;
    void			checkPosition(BinID&);

    void			knotMoved(CallBacker*);
    void			knotNrChanged(CallBacker*);
    void			rightClicked(CallBacker*);

    ObjectSet< TypeSet<BinID> > bidsset;
    ObjectSet<SeisTrc>		cache;
    bool			ismanip;

    static const char*		trackstr;
    static const char*		nrknotsstr;
    static const char*		knotprefix;
    static const char*		depthintvstr;

};

};


#endif

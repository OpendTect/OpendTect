#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.4 2003-01-27 13:15:53 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class CubeSampling;
class Coord;
class SeisTrc;

namespace visBase { class RandomTrack; class VisColorTab; };

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
    void			addKnot(const Coord&);
    void			addKnots(TypeSet<Coord>);
    void			insertKnot(int,const Coord&);
    void			setKnotPos(int,const Coord&);
    Coord			getKnotPos(int) const;
    Coord			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<Coord>&);
    void			removeKnot(int);
    void			removeAllKnots();

    bool			updateAtNewPos();
    void			getDataPositions(TypeSet<BinID>&);
    bool			putNewData(ObjectSet<SeisTrc>&);

    void			showDragger(bool);
    bool			isDraggerShown() const;
    void			turnOn(bool);
    bool			isOn() const;

    void			setColorTab(visBase::VisColorTab&);
    visBase::VisColorTab&	getColorTab();

    void                        setMaterial( visBase::Material* );
    const visBase::Material*    getMaterial() const;
    visBase::Material*          getMaterial();

    SoNode*			getData();

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~RandomTrackDisplay();

    visBase::RandomTrack*	track;

    void			select();
    void			deSelect();

    AttribSelSpec&		as;

    TypeSet<int>		trcspersection;
    bool			selected_;
    bool			manipulated;
    bool			succeeded_;
};

};


#endif

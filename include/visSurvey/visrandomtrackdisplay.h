#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visrandomtrackdisplay.h,v 1.2 2003-01-21 09:17:06 nanne Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class AttribSelSpec;
class CubeSampling;
class ColorTable;
class Coord;
class SeisTrc;

namespace visBase { class RandomTrack; };

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

    void			setAttribSelSpec(AttribSelSpec&);
    AttribSelSpec&		getAttribSelSpec();
    const AttribSelSpec&	getAttribSelSpec() const;

    void			setDepthInterval(const Interval<float>&);
    const Interval<float>&	getDepthInterval() const;

    int				nrKnots() const;
    void			addKnot(const Coord&);
    void			addKnots(TypeSet<Coord>);
    void			insertKnot(int,const Coord&);
    void			setKnotPos(int,const Coord&);
    Coord			getKnotPos(int) const;
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

    void			setColorTable(const ColorTable&);
    const ColorTable&		getColorTable() const;
    void			setClipRate(float);
    float			clipRate() const;
    void			setAutoscale(bool);
    bool			autoScale() const;
    void			setDataRange(const Interval<float>&);
    Interval<float>		getDataRange() const;

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

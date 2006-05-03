#ifndef visseis2ddisplay_h
#define visseis2ddisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id: visseis2ddisplay.h,v 1.2 2006-05-03 18:54:19 cvskris Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"

class ColorTable;
class CubeSampling;
class SeisTrcInfo;

namespace Attrib 
{ 
    class SelSpec; 
    class Data2DHolder;
}

namespace visBase
{
    class Material;
    class Text2;
    class Transformation;
    class TriangleStripSet;
    class Texture2;
};

namespace PosInfo { class Line2DData; }

namespace visSurvey
{

/*!\brief Used for displaying a 2D line

*/

class Seis2DDisplay :  public visBase::VisualObjectImpl, public SurveyObject
{
public:
    static Seis2DDisplay*	create()
				mCreateDataObj(Seis2DDisplay);

    void			setLineName(const char*);

    void			setSelSpec(int attrib, const Attrib::SelSpec&);
    const Attrib::SelSpec*	getSelSpec(int attrib) const;

    void			setCubeSampling(CubeSampling);
    CubeSampling		getCubeSampling() const;

    void			setZLimits(Interval<float> zrg) 
    						{zlimits = zrg; iszlset = true;}
    Interval<float>		getZLimits() {return zlimits;}
    bool			isZLimitSet() {return iszlset;}

    void			setGeometry(const PosInfo::Line2DData&,
	    				    const CubeSampling* cs=0);
    void			setTraceData( const Attrib::Data2DHolder& );
    void			clearTexture();

    bool			allowPicks() const		{ return true; }
    bool			allowMaterialEdit() const	{ return true; }

    void			showLineName(bool);
    bool			lineNameShown() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate() const;

    void			setDisplayTransformation(
	    					visBase::Transformation*);
    visBase::Transformation*	getDisplayTransformation();

    float			calcDist(const Coord3&) const;

    int				nrResolutions() const		{ return 3; }
    int				getResolution() const;
    void			setResolution(int);

    SurveyObject::AttribFormat 	getAttributeFormat() const;

    void			getColTabDef(ColorTable&,Interval<float>& scl,
	    				     float& cliprate) const;
    void			setColTabDef(const ColorTable&,
	    				     const Interval<float>& scl,
					     float cliprate);
    int				getColTabID(int) const;
    const TypeSet<float>*	getHistogram(int) const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,float&,
	    					BufferString&) const;
    void			snapToTracePos(Coord3&);

    void			setLineSetID(const MultiID& mid)
				{ linesetid = mid; }
    const MultiID&		lineSetID() const	{ return linesetid; }

    NotifierAccess*		getMovementNotification()	
    				{ return &geomchanged; }

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

protected:
				~Seis2DDisplay();

    void			setPlaneCoordinates(const TypeSet<Coord>&,
						    const Interval<float>&);
    void			setStrip(const TypeSet<Coord>&,
	    				 const Interval<float>&,
					 int stripidx, const Interval<int>& );
    void			addLineName();
    void			setData(const Attrib::Data2DHolder&);
    void			getNearestTrace(const Coord3&,int& idx,
						float& sqdist) const;

    const Attrib::Data2DHolder*	cache_;

    Attrib::SelSpec&		as;
    CubeSampling&		cs;
    MultiID			linesetid;
    Interval<float>		zlimits;
    bool			iszlset;

    ObjectSet<visBase::TriangleStripSet>	planes;
    visBase::Texture2*				texture;

    visBase::Transformation*	transformation;
    visBase::Text2*		linename;

    Notifier<Seis2DDisplay>	geomchanged;

    static const char*		linesetidstr;
    static const char*		textureidstr;
};

};


#endif

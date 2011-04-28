#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismarchingcubessurfacedisplay.h,v 1.26 2011-04-28 07:00:12 cvsbert Exp $
________________________________________________________________________


-*/

#include "attribsel.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class MarchingCubesSurfaceEditor;
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class Dragger;
    class Ellipsoid;
    class IndexedPolyLine;
    class MarchingCubesSurface;
    class PickStyle;
    class Transformation;
    class InvisibleLineDragger;
};


namespace EM { class MarchingCubesSurface; }


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

mClass MarchingCubesDisplay : public visBase::VisualObjectImpl,
			      public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID		getMultiID() const;
    bool		isInlCrl() const	{ return true; }

    bool		hasColor() const	{ return true; }
    Color		getColor() const;
    void		setColor(Color);
    bool		allowMaterialEdit() const { return true; }
    NotifierAccess*	materialChange();

    void		useTexture(bool yn);
    bool		usesTexture() const;

    void		setDisplayTransformation(mVisTrans*);
    mVisTrans*		getDisplayTransformation();
    void		setRightHandSystem(bool);


    bool		setVisSurface(visBase::MarchingCubesSurface*);
    			//!<Creates an EMObject for it.
    bool		setEMID(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID	getEMID() const;

    const char*		errMsg() const { return errmsg_.str(); }

    SurveyObject::AttribFormat	getAttributeFormat(int) const;
    int				nrAttribs() const;
    bool			canAddAttrib(int) const;
    bool			canRemoveAttrib() const;
    bool			canHandleColTabSeqTrans(int) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int,int) const;

    void			setColTabMapperSetup(int,
	    				const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void                	setColTabSequence(int,const ColTab::Sequence&,
	    					  TaskRunner*);
    void                   	setSelSpec(int,const Attrib::SelSpec&);
    const Attrib::SelSpec*	getSelSpec(int attrib) const;

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData( int attrib,
					    const DataPointSet*, TaskRunner*);

protected:

    virtual			~MarchingCubesDisplay();
    void			updateVisFromEM(bool onlyshape,TaskRunner*);
    virtual void		fillPar(IOPar&,TypeSet<int>& saveids) const;
    virtual int			usePar(const IOPar&);
    void			materialChangeCB(CallBacker*);

    void			getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
	     			    Coord3& xyzpos, BufferString& val,
	     			    BufferString& info) const;

    visBase::MarchingCubesSurface*	displaysurface_;
    EM::MarchingCubesSurface*		emsurface_;
    Attrib::SelSpec			selspec_;
    const DataPointSet*			cache_;
    
    static const char*	sKeyEarthModelID()	{ return "EM ID"; }
    static const char*	sKeyAttribSelSpec()	{ return "Attrib SelSpec"; }
    static const char*	sKeyColTabMapper()	{ return "Coltab mapper"; }
    static const char*	sKeyColTabSequence()	{ return "Coltab sequence"; }   
};

};


#endif

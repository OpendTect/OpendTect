#ifndef vishorizondisplay_h
#define vishorizondisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizondisplay.h,v 1.18 2007-10-12 19:14:34 cvskris Exp $
________________________________________________________________________


-*/

#include "visemobjdisplay.h"

class Executor;

namespace visBase { class ParametricSurface; }

namespace visSurvey
{


class HorizonDisplay : public EMObjectDisplay
{
public:
    static HorizonDisplay*	create()
				mCreateDataObj( HorizonDisplay );
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;


    bool			setEMObject(const EM::ObjectID&);
    bool			updateFromEM();
    void			updateFromMPE();

    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;

    void			useTexture(bool yn,bool trigger=false);
    bool			usesTexture() const;

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			getOnlyAtSectionsDisplay() const;

    void			displaySurfaceData(int attrib,int auxdatanr);

    virtual bool		canHaveMultipleAttribs() const;
    virtual int			nrTextures(int attrib) const;
    virtual void		selectTexture(int attrib,int textureidx);
    virtual int			selectedTexture(int attrib) const;

    SurveyObject::AttribFormat	getAttributeFormat() const;
    Pol2D3D                     getAllowedDataType() const      
    				{ return Both2DAnd3D; }
    
    int				nrAttribs() const;
    bool			addAttrib();
    bool			canAddAttrib() const;
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;
    void			enableAttrib(int attrib, bool yn);
    bool			isAttribEnabled(int attrib) const;
    void			setAngleFlag(int attrib,bool yn);
    bool			isAngle(int attrib) const;
    void			allowShading(bool);
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    void			setDepthAsAttrib(int);

    bool			allowMaterialEdit() const { return true; }
    bool			hasColor() const;

    EM::SectionID		getSectionID(int visid) const;

    void			getRandomPos(ObjectSet<BinIDValueSet>&) const;
    void			getRandomPosCache(int,
	    				ObjectSet<const BinIDValueSet>&) const;
    void			setRandomPosData( int,
					   const ObjectSet<BinIDValueSet>*);

    bool			hasStoredAttrib( int attrib ) const;

    int				nrResolutions() const;
    BufferString		getResolutionName(int) const;
    int				getResolution() const;
    void			setResolution(int);
    				/*!< 0 is automatic */

    bool			allowPicks() const	{ return true; }
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,
	    					BufferString& val,
						BufferString& info) const;
    float			calcDist(const Coord3&) const;
    float			maxDist() const;

    int				getColTabID(int) const;

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);

    bool			usesWireframe() const;
    void			useWireframe(bool);

    void			setEdgeLineRadius(float);
    float			getEdgeLineRadius() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    				~HorizonDisplay();
    void			removeEMStuff();

    EM::PosID			findClosestNode(const Coord3&) const;

    void			removeSectionDisplay(const EM::SectionID&);
    visBase::VisualObject*	createSection(const EM::SectionID&) const;
    bool			addSection(const EM::SectionID&);
    bool			addEdgeLineDisplay(const EM::SectionID&);
    void			emChangeCB(CallBacker*);
    void			emEdgeLineChangeCB(CallBacker*);
    void			edgeLineRightClickCB(CallBacker*);
    void			updateIntersectionLines(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			updateSectionSeeds(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj );

    mVisTrans*				translation_;

    ObjectSet<EdgeLineSetDisplay>	edgelinedisplays_;

    ObjectSet<visBase::ParametricSurface> sections_;
    TypeSet<EM::SectionID>		sids_;

    ObjectSet<visBase::IndexedPolyLine>	intersectionlines_;
    TypeSet<int>			intersectionlineids_;
    TypeSet<int>			intersectionlinevoi_;

    ZAxisTransform*			zaxistransform_;

    StepInterval<int>			parrowrg_;
    StepInterval<int>			parcolrg_;


    bool				usestexture_;
    bool				useswireframe_;
    int					resolution_;
    int					curtextureidx_;
    float				edgelineradius_;

    ObjectSet<Attrib::SelSpec>		as_;
    ObjectSet<visBase::VisColorTab>	coltabs_;
    BoolTypeSet				enabled_;
    bool				validtexture_;

    static const char*			sKeyTexture;
    static const char*			sKeyColorTableID;
    static const char*			sKeyShift;
    static const char*			sKeyWireFrame;
    static const char*			sKeyResolution;
    static const char*			sKeyEdgeLineRadius;
    static const char*			sKeyRowRange;
    static const char*			sKeyColRange;
};


};

#endif



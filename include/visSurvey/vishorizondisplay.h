#ifndef vishorizondisplay_h
#define vishorizondisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizondisplay.h,v 1.40 2009-06-02 21:40:49 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visemobjdisplay.h"
#include "coltabmapper.h"
#include "coltabsequence.h"

class Executor;
namespace ColTab{ class Sequence; struct MapperSetup; }
namespace EM { class Horizon3D; }
namespace visBase { class HorizonSection; class IndexedShape; }

namespace visSurvey
{


mClass HorizonDisplay : public EMObjectDisplay
{
public:
    static HorizonDisplay*	create()
				mCreateDataObj( HorizonDisplay );
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    bool			setDataTransform(ZAxisTransform*);
    const ZAxisTransform*	getDataTransform() const;

    bool			setEMObject(const EM::ObjectID&,TaskRunner*);
    bool			updateFromEM(TaskRunner*);
    void			updateFromMPE();

    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;

    void			useTexture(bool yn,bool trigger=false);
    bool			usesTexture() const;
    bool			showingTexture() const;

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			getOnlyAtSectionsDisplay() const;

    void			displaySurfaceData(int attrib,int auxdatanr);

    virtual bool		canHaveMultipleAttribs() const;
    virtual int			nrTextures(int attrib) const;
    virtual void		selectTexture(int attrib,int textureidx);
    virtual int			selectedTexture(int attrib) const;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;
    Pol2D3D                     getAllowedDataType() const      
    				{ return Both2DAnd3D; }
    
    int				nrAttribs() const;
    bool			addAttrib();
    bool			canAddAttrib(int nrattribstoadd=1) const;
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    
    void			allowShading(bool);
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    void			setDepthAsAttrib(int);
    void			createAndDispDataPack(int,const DataPointSet*,
	    					TaskRunner*);
    bool                        setDataPackID(int attrib,DataPack::ID,
	    				      TaskRunner*);
    DataPack::ID                getDataPackID(int attrib) const;
    virtual DataPackMgr::ID     getDataPackMgrID() const
				{ return DataPackMgr::FlatID(); }

    bool			allowMaterialEdit() const 	{ return true; }
    bool			hasColor() const		{ return true; }

    EM::SectionID		getSectionID(int visid) const;

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			getRandomPosCache(int,DataPointSet&) const;
    void			setRandomPosData( int,const DataPointSet*,
	    					 TaskRunner*);

    void			setLineStyle(const LineStyle&);
    				/*!<If ls is solid, a 3d shape will be used,
				    otherwise 'flat' lines. */
    bool			hasStoredAttrib(int attrib) const;
    bool			hasDepth(int attrib) const;

    int				nrResolutions() const;
    BufferString		getResolutionName(int) const;
    int				getResolution() const;
    void			setResolution(int);
    				/*!< 0 is automatic */

    bool			allowPicks() const		{ return true; }
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,
	    					BufferString& val,
						BufferString& info) const;
    float			calcDist(const Coord3&) const;
    float			maxDist() const;

    const ColTab::Sequence*	getColTabSequence(int attr) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int attr,
				    const ColTab::Sequence&,TaskRunner*);
    const ColTab::MapperSetup*	getColTabMapperSetup(int attr) const;
    void			setColTabMapperSetup(int attr,
				    const ColTab::MapperSetup&,TaskRunner*);

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);
    void			setAttribShift(int attr,const TypeSet<float>&);

    bool			usesWireframe() const;
    void			useWireframe(bool);

    void			setEdgeLineRadius(float);
    float			getEdgeLineRadius() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    bool                        canBDispOn2DViewer() const      { return true; }
    bool                        isVerticalPlane() const		{ return false;}
        
protected:
    				~HorizonDisplay();
    void			removeEMStuff();

    EM::PosID			findClosestNode(const Coord3&) const;

    bool			shouldUseTexture() const;

    void			removeSectionDisplay(const EM::SectionID&);
    visBase::VisualObject*	createSection(const EM::SectionID&) const;
    bool			addSection(const EM::SectionID&,TaskRunner*);
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
    void			updateSingleColor();

    bool				allowshading_;
    mVisTrans*				translation_;

    ObjectSet<EdgeLineSetDisplay>	edgelinedisplays_;

    ObjectSet<visBase::HorizonSection> sections_;
    TypeSet<EM::SectionID>		sids_;

    ObjectSet<visBase::IndexedShape>	intersectionlines_;
    float				maxintersectionlinethickness_;
    TypeSet<int>			intersectionlineids_;
    TypeSet<int>			intersectionlinevoi_;

    ZAxisTransform*			zaxistransform_;

    StepInterval<int>			parrowrg_;
    StepInterval<int>			parcolrg_;

    TypeSet<ColTab::MapperSetup>	coltabmappersetups_;//for each channel
    TypeSet<ColTab::Sequence>		coltabsequences_;  //for each channel

    bool				usestexture_;
    bool				useswireframe_;
    int					resolution_;
    int					curtextureidx_;
    float				edgelineradius_;

    ObjectSet<Attrib::SelSpec>		as_;
    TypeSet<DataPack::ID>		datapackids_;
    BoolTypeSet				enabled_;
    TypeSet<int>			curshiftidx_;
    TypeSet< TypeSet<float> >		shifts_;
    ObjectSet<BufferStringSet>		userrefs_;
    bool				validtexture_;

    static const char*			sKeyTexture();
    static const char*			sKeyShift();
    static const char*			sKeyWireFrame();
    static const char*			sKeyResolution();
    static const char*			sKeyEdgeLineRadius();
    static const char*			sKeyRowRange();
    static const char*			sKeyColRange();
};


};

#endif



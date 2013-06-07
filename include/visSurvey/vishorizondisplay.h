#ifndef vishorizondisplay_h
#define vishorizondisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id$
________________________________________________________________________


-*/

#include "visemobjdisplay.h"
#include "coltabmapper.h"
#include "coltabsequence.h"

class Executor;
namespace ColTab{ class Sequence; class MapperSetup; }
namespace EM { class Horizon3D; }
namespace visBase
{
    class HorizonSection;
    class IndexedShape;
    class TextureChannel2RGBA;
}


namespace visSurvey
{


mClass HorizonDisplay : public EMObjectDisplay
{
public:
    static HorizonDisplay*	create()
				mCreateDataObj( HorizonDisplay );
    void			setDisplayTransformation(const mVisTrans*);
    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			enableTextureInterpolation(bool);
    bool			textureInterpolationEnabled() const
				{ return enabletextureinterp_; }
    bool			canEnableTextureInterpolation() const
				{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    visBase::Material*		getMaterial();
    void			setIntersectLineMaterial(visBase::Material*);

    bool			setEMObject(const EM::ObjectID&,TaskRunner*);
    bool			updateFromEM(TaskRunner*);
    void			updateFromMPE();

    StepInterval<int>		geometryRowRange() const;
    StepInterval<int>		geometryColRange() const;
    visBase::HorizonSection*	getHorizonSection(const EM::SectionID&);
    TypeSet<EM::SectionID>	getSectionIDs() const	{ return sids_; }

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
    bool			canRemoveAttrib() const;
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
    void			setResolution(int,TaskRunner*);
    				/*!< 0 is automatic */

    bool			allowsPicks() const		{ return true; }
    void			getMousePosInfo(const visBase::EventInfo& e,
	    					IOPar& i ) const
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					Coord3&,
	    					BufferString& val,
						BufferString& info) const;
    float			calcDist(const Coord3&) const;
    float			maxDist() const;

    const ColTab::Sequence*	getColTabSequence(int attr) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int attr,
				    const ColTab::Sequence&,TaskRunner*);
    const ColTab::MapperSetup*	getColTabMapperSetup(int attr,int v=0) const;
    void			setColTabMapperSetup(int attr,
				    const ColTab::MapperSetup&,TaskRunner*);
    const TypeSet<float>*	getHistogram(int) const;

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);

    bool			usesWireframe() const;
    void			useWireframe(bool);

    void			setEdgeLineRadius(float);
    float			getEdgeLineRadius() const;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    visBase::TextureChannel2RGBA* getChannels2RGBA();
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    bool                        canBDispOn2DViewer() const      { return true; }
    bool                        isVerticalPlane() const		{ return false;}
    
    bool			shouldUseTexture() const;
    void			setAttribShift(int channel,
	    				       const TypeSet<float>& shifts);
    				/*!<Gives the shifts for all versions of an 
				    attrib. */

    const ObjectSet<visBase::IndexedShape>& getIntersectionLines() const;

    void			displayIntersectionLines(bool);
    bool			displaysIntersectionLines() const;
    const visBase::HorizonSection*	getSection(int id) const;

    static HorizonDisplay*	getHorizonDisplay(const MultiID&);

    void			doOtherObjectsMoved(
	    				const ObjectSet<const SurveyObject>&,
					int whichobj );
protected:
    				~HorizonDisplay();
    void			removeEMStuff();

    EM::PosID			findClosestNode(const Coord3&) const;

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

    void			traverseLine(bool oninline,
				    const CubeSampling& cs,
				    EM::SectionID sid,
				    visBase::IndexedShape*,
				    int& cii,
				    visBase::DataObjectGroup*) const;
    void			drawHorizonOnRandomTrack(const TypeSet<Coord>&,
				    const StepInterval<float>&,
				    const EM::SectionID&,
				    visBase::IndexedShape*, int&,
				    visBase::DataObjectGroup*) const;


    bool				allowshading_;
    mVisTrans*				translation_;

    ObjectSet<EdgeLineSetDisplay>	edgelinedisplays_;

    ObjectSet<visBase::HorizonSection> sections_;
    TypeSet<EM::SectionID>		sids_;

    ObjectSet<visBase::IndexedShape>	intersectionlines_;
    ObjectSet<visBase::DataObjectGroup>	intersectionpointsets_;
    float				maxintersectionlinethickness_;
    TypeSet<int>			intersectionlineids_;
    TypeSet<int>			intersectionlinevoi_;
    visBase::Material*			intersectionlinematerial_;

    StepInterval<int>			parrowrg_;
    StepInterval<int>			parcolrg_;

    TypeSet<ColTab::MapperSetup>	coltabmappersetups_;//for each channel
    TypeSet<ColTab::Sequence>		coltabsequences_;  //for each channel
    bool				enabletextureinterp_;

    bool				usestexture_;
    bool				useswireframe_;
    int					resolution_;
    int					curtextureidx_;
    float				edgelineradius_;

    bool				displayintersectionlines_;

    ObjectSet<Attrib::SelSpec>		as_;
    TypeSet<DataPack::ID>		datapackids_;
    BoolTypeSet				enabled_;
    TypeSet<int>			curshiftidx_;
    ObjectSet< TypeSet<float> >		shifts_;
    bool				validtexture_;

    static const char*			sKeyTexture();
    static const char*			sKeyShift();
    static const char*			sKeyWireFrame();
    static const char*			sKeyResolution();
    static const char*			sKeyEdgeLineRadius();
    static const char*			sKeyRowRange();
    static const char*			sKeyColRange();
    static const char*			sKeyIntersectLineMaterialID();
};


};

#endif



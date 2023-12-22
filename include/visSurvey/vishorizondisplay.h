#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "visemobjdisplay.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "factory.h"
#include "uistring.h"

namespace ColTab{ class Sequence; class MapperSetup; }
namespace visBase
{
    class HorizonSection;
    class HorizonTextureHandler;
    class MarkerSet;
    class PointSet;
    class TextureChannel2RGBA;
    class VertexShape;
    class PolyLine;
}

namespace visSurvey
{

mExpClass(visSurvey) HorizonDisplay : public EMObjectDisplay
{ mODTextTranslationClass(HorizonDisplay)

    friend class HorizonPathIntersector;
    struct IntersectionData;
public:
				HorizonDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,HorizonDisplay,
				    "HorizonDisplay",
				    toUiString(sFactoryKeyword()))

    void			setDisplayTransformation(
					    const mVisTrans*) override;
    void			setSceneEventCatcher(
					    visBase::EventCatcher*) override;

    void			enableTextureInterpolation(bool) override;
    bool			textureInterpolationEnabled() const override
				{ return enabletextureinterp_; }
    bool			canEnableTextureInterpolation() const override
				{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;

    void			setIntersectLineMaterial(visBase::Material*);

    bool			setEMObject(const EM::ObjectID&,
					    TaskRunner*) override;
    bool			updateFromEM(TaskRunner*) override;
    void			updateFromMPE() override;

    StepInterval<int>		geometryRowRange() const;
    StepInterval<int>		geometryColRange() const;
    visBase::HorizonSection*	getHorizonSection();
    const visBase::HorizonSection* getHorizonSection() const;
    TypeSet<EM::SectionID>	getSectionIDs() const	{ return sids_; }

    void			useTexture(bool yn,bool trigger=false) override;
    bool			canShowTexture() const override;

    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    void			displaySurfaceData(int attrib,int auxdatanr);

    bool			canHaveMultipleAttribs() const override;
    int				nrTextures(int attrib) const override;
    void			selectTexture(int attrib,
					      int textureidx) override;
    int				selectedTexture(int attrib) const override;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const override;
    OD::Pol2D3D			getAllowedDataType() const override
				{ return OD::Both2DAnd3D; }

    int				nrAttribs() const override;
    bool			addAttrib() override;
    bool			canAddAttrib(
					int nrattribstoadd=1) const override;
    bool			removeAttrib(int attrib) override;
    bool			canRemoveAttrib() const override;
    bool			swapAttribs(int attrib0,int attrib1) override;
    void			setAttribTransparency(int,
						      unsigned char) override;
    unsigned char		getAttribTransparency(int) const override;
    void			enableAttrib(int attrib,bool yn) override;
    bool			isAttribEnabled(int attrib) const override;
    bool			hasSingleColorFallback() const override
				{ return true; }
    OD::Color			singleColor() const;

    void			allowShading(bool) override;
    const Attrib::SelSpec*	getSelSpec(int channel,
					   int version=-1) const override;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const override;

    void			setSelSpec(int,const Attrib::SelSpec&) override;
    void			setSelSpecs(int attrib,
				    const TypeSet<Attrib::SelSpec>&) override;
    void			setDepthAsAttrib(int);
    void			setDisplayDataPackIDs(int attrib,
					const TypeSet<DataPackID>&);
    DataPackID			getDataPackID(int attrib) const override;
    DataPackID			getDisplayedDataPackID(
						int attrib )const override;
    virtual DataPackMgr::MgrID	getDataPackMgrID() const override
				{ return DataPackMgr::FlatID(); }

    bool			allowMaterialEdit() const override
				{ return true; }
    bool			hasColor() const override	{ return true; }
    bool			usesColor() const override;

    EM::SectionID		getSectionID(VisID visid) const override;

    void			getRandomPos(DataPointSet&,
						TaskRunner*) const override;
    void			getRandomPosCache(int,
						  DataPointSet&) const override;
    void			setRandomPosData(int,const DataPointSet*,
						 TaskRunner*) override;

    void			setLineStyle(const OD::LineStyle&) override;
				/*!<If ls is solid, a 3d shape will be used,
				    otherwise 'flat' lines. */
    bool			hasStoredAttrib(int attrib) const;
    bool			hasDepth(int attrib) const;

    int				nrResolutions() const override;
    BufferString		getResolutionName(int) const override;
    int				getResolution() const override;
    bool			displaysSurfaceGrid() const;
    void			displaysSurfaceGrid(bool);
    void			setResolution(int,TaskRunner*) override;
				/*!< 0 is automatic */

    bool			allowsPicks() const override	{ return true; }
    void			getMousePosInfo( const visBase::EventInfo& e,
						 IOPar& i ) const override
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
					    Coord3&,BufferString& val,
					    BufferString& info) const override;
    float			calcDist(const Coord3&) const override;
    float			maxDist() const override;

    const ColTab::Sequence*	getColTabSequence(int attr) const override;
    bool			canSetColTabSequence() const override;
    void			setColTabSequence(int attr,
				    const ColTab::Sequence&,
				    TaskRunner*) override;
    const ColTab::MapperSetup*	getColTabMapperSetup(int attr,
						     int v=0) const override;
    void			setColTabMapperSetup(int attr,
						     const ColTab::MapperSetup&,
						     TaskRunner*) override;
    const TypeSet<float>*	getHistogram(int) const override;

    Coord3			getTranslation() const override;
    void			setTranslation(const Coord3&) override;

    bool			setChannels2RGBA(
					visBase::TextureChannel2RGBA*) override;
    visBase::TextureChannel2RGBA* getChannels2RGBA() override;
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canBDispOn2DViewer() const override
				{ return false; }
    bool			isVerticalPlane() const override
				{ return false; }

    void			setAttribShift(int channel,
					       const TypeSet<float>& shifts);
				/*!<Gives the shifts for all versions of an
				    attrib. */

    const ObjectSet<visBase::VertexShape>& getIntersectionLines() const;
					/*!<This function will be deleted*/
    int				getIntersectionDataSize() const
				{ return intersectiondata_.size(); }
    const visBase::VertexShape* getLine(int) const;
    void			displayIntersectionLines(bool);
    bool			displaysIntersectionLines() const;
    const visBase::HorizonSection*	getSection(int id) const;

    static HorizonDisplay*	getHorizonDisplay(const MultiID&);

    void			doOtherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					VisID whichobj) override;
    void			setPixelDensity(float) override;

    void			setSectionDisplayRestore(bool);
    BufferString		getSectionName(int secidx) const;

    void			selectParent(const TrcKey&);
    void			selectChildren(const TrcKey&);
    void			selectChildren();
    void			showParentLine(bool);
    void			showSelections(bool);
    void			showLocked(bool);
    bool			lockedShown() const;
    void			clearSelections() override;
    void			updateAuxData() override;
    bool			canBeRemoved() const override;

// Deprecated public functions
    mDeprecated("Use without SectionID")
    visBase::HorizonSection*	getHorizonSection(const EM::SectionID&)
				{ return getHorizonSection(); }
    mDeprecated("Use without SectionID")
    const visBase::HorizonSection*
				getHorizonSection(const EM::SectionID&) const
				{ return getHorizonSection(); }

private:
				~HorizonDisplay();

    void			removeEMStuff() override;

    EM::PosID			findClosestNode(const Coord3&) const override;
    void			createDisplayDataPacks(
					int attrib,const DataPointSet*);

    void			removeSectionDisplay(
						const EM::SectionID&) override;
    visBase::VisualObject*	createSection(const EM::SectionID&) const;
    bool			addSection(const EM::SectionID&,
					   TaskRunner*) override;
    void			emChangeCB(CallBacker*) override;
    int				getChannelIndex(const char* nm) const;
    void			updateIntersectionLines(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj );
    void			updateSectionSeeds(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj );
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    VisID whichobj) override;
    void			updateSingleColor();

    void			calculateLockedPoints();
    void			initSelectionDisplay(bool erase);
    void			updateSelections() override;
    void			handleEmChange(const EM::EMObjectCallbackData&);
    void			updateLockedPointsColor();

    bool				allowshading_;
    mVisTrans*				translation_;
    Coord3				translationpos_;

    ObjectSet<visBase::HorizonSection>  sections_;
    TypeSet<BufferString>		secnames_;
    TypeSet<EM::SectionID>		sids_;

    struct IntersectionData
    {
				IntersectionData(const OD::LineStyle&);
				~IntersectionData();
	void			addLine(const TypeSet<Coord3>&);
	void			clear();

	void			setPixelDensity(float);
	void			setDisplayTransformation(const mVisTrans*);
	void			updateDataTransform(const TrcKeyZSampling&,
						   ZAxisTransform*);
	void			setSceneEventCatcher(visBase::EventCatcher*);
	void			setMaterial(visBase::Material*);
	RefMan<visBase::VertexShape> setLineStyle(const OD::LineStyle&);
				//Returns old line if replaced


	visBase::VertexShape*		line_;
	visBase::MarkerSet*		markerset_;
	ZAxisTransform*			zaxistransform_;
	int				voiid_;
	VisID				objid_;
    };

    IntersectionData*		getOrCreateIntersectionData(
				     ObjectSet<IntersectionData>& pool );
				//!<Return data from pool or creates new

    void			traverseLine(const TrcKeyPath&,
					     const TypeSet<Coord>&,
					     const Interval<float>& zrg,
					     IntersectionData&) const;
				/*!<List of coordinates may be empty, coords
				    will then be fetched from trckeys. */
    void			drawHorizonOnZSlice(const TrcKeyZSampling&,
					     IntersectionData&) const;

    bool			isValidIntersectionObject(
				   const ObjectSet<const SurveyObject>&,
				   int& objidx,VisID objid) const;
				/*!<Check if the active object is one of
				planedata, z-slice, 2dline,..., if it is
				get the the idx in the stored object
				collection.*/
    ManagedObjectSet<IntersectionData>	intersectiondata_;
					//One per object we intersect with

    float				maxintersectionlinethickness_;
    visBase::Material*			intersectionlinematerial_;

    visBase::PointSet*			selections_;
    visBase::PointSet*			lockedpts_;
    visBase::PointSet*			sectionlockedpts_;
    visBase::VertexShape*		parentline_;

    StepInterval<int>			parrowrg_;
    StepInterval<int>			parcolrg_;

    TypeSet<ColTab::MapperSetup>	coltabmappersetups_;//for each channel
    TypeSet<ColTab::Sequence>		coltabsequences_;  //for each channel
    bool				enabletextureinterp_;

    char				resolution_;
    int					curtextureidx_;

    bool				displayintersectionlines_;

    ObjectSet<TypeSet<Attrib::SelSpec> > as_;
    ObjectSet<TypeSet<DataPackID> >	dispdatapackids_;
    BoolTypeSet				enabled_;
    TypeSet<int>			curshiftidx_;
    ObjectSet< TypeSet<float> >		shifts_;
    bool				displaysurfacegrid_;

    TypeSet<EM::SectionID>		oldsectionids_;
    TypeSet<StepInterval<int> >		olddisplayedrowranges_;
    TypeSet<StepInterval<int> >		olddisplayedcolranges_;

    ObjectSet<visBase::HorizonTextureHandler> oldhortexhandlers_;
    Threads::Mutex*			locker_;
    bool				showlock_;

    static const char*			sKeyTexture();
    static const char*			sKeyShift();
    static const char*			sKeyResolution();
    static const char*			sKeyRowRange();
    static const char*			sKeyColRange();
    static const char*			sKeyIntersectLineMaterialID();
    static const char*			sKeySurfaceGrid();
    static const char*			sKeySectionID();
    static const char*			sKeyZValues();
};

} // namespace visSurvey

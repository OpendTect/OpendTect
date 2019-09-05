#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
________________________________________________________________________


-*/

#include "visemobjdisplay.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "factory.h"
#include "uistring.h"

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

public:
				HorizonDisplay();
				mDefaultFactoryInstantiation(
				    SurveyObject,HorizonDisplay,
				    "HorizonDisplay",
				    toUiString(sFactoryKeyword()))
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

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

    bool			setEMObject(const DBKey&,TaskRunner*);
    bool			updateFromEM(TaskRunner*);
    void			updateFromMPE();

    StepInterval<int>		geometryRowRange() const;
    StepInterval<int>		geometryColRange() const;
    visBase::HorizonSection*	getHorizonSection(const EM::SectionID&);
    const visBase::HorizonSection*
				getHorizonSection(const EM::SectionID&) const;
    TypeSet<EM::SectionID>	getSectionIDs() const	{ return sids_; }

    void			useTexture(bool yn,bool trigger=false);
    bool			usesTexture() const;
    bool			showsTexture() const;
    bool			canShowTexture() const;

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			displayedOnlyAtSections() const;

    void			displaySurfaceData(int attrib,int auxdatanr);

    virtual bool		canHaveMultipleAttribs() const;

    virtual bool		canHaveMultipleTextures() const { return true; }
    virtual int			nrTextures(int attrib) const;
    virtual void		selectTexture(int attrib,int textureidx);
    virtual int			selectedTexture(int attrib) const;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;
    OD::Pol2D3D			getAllowedDataType() const
				{ return OD::Both2DAnd3D; }

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
    bool			hasSingleColorFallback() const	{ return true; }
    Color			getColor() const;

    void			allowShading(bool);
    const Attrib::SelSpec*	getSelSpec(int channel,int version=0) const;
    const Attrib::SelSpecList*	getSelSpecs(int attrib) const;
    void			setSelSpecs(int attrib,
					    const Attrib::SelSpecList&);
    void			setDepthAsAttrib(int);
    void			setDisplayDataPackIDs(int attrib,
					const TypeSet<DataPack::ID>&);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::FlatID(); }

    bool			allowMaterialEdit() const	{ return true; }
    bool			hasColor() const		{ return true; }
    bool			usesColor() const;

    EM::SectionID		getSectionID(int visid) const;

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			getRandomPosCache(int,DataPointSet&) const;
    void			setRandomPosData( int,const DataPointSet*,
						 const TaskRunnerProvider&);

    void			setLineStyle(const OD::LineStyle&);
				/*!<If ls is solid, a 3d shape will be used,
				    otherwise 'flat' lines. */
    bool			hasStoredAttrib(int attrib) const;
    bool			hasDepth(int attrib) const;

    int				nrResolutions() const;
    uiWord			getResolutionName(int) const;
    int				getResolution() const;
    bool			displaysSurfaceGrid() const;
    void			displaysSurfaceGrid(bool);
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

    const ColTab::Sequence&	getColTabSequence(int attr) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int attr,
				    const ColTab::Sequence&,TaskRunner*);
    const ColTab::Mapper&	getColTabMapper(int) const;
    void			setColTabMapper(int,const ColTab::Mapper&,
						TaskRunner*);

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    visBase::TextureChannel2RGBA* getChannels2RGBA();
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const;

    bool			canBDispOn2DViewer() const	{ return false;}
    bool			isVerticalPlane() const		{ return false;}

    void			setAttribShift(int channel,
					       const TypeSet<float>& shifts);
				/*!<Gives the shifts for all versions of an
				    attrib. */

    const ObjectSet<visBase::VertexShape>& getIntersectionLines() const;

    void			displayIntersectionLines(bool);
    bool			displaysIntersectionLines() const;
    const visBase::HorizonSection*	getSection(int id) const;

    static HorizonDisplay*	getHorizonDisplay(const DBKey&);

    void			doOtherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					int whichobj );
    virtual void		setPixelDensity(float);

    void			setSectionDisplayRestore(bool);
    BufferString		getSectionName(int secidx) const;

    void			selectParent(const TrcKey&);
    void			selectChildren();
    void			showParentLine(bool);
    void			showSelections(bool);
    void			showLocked(bool);
    bool			lockedShown() const;
    virtual void		clearSelections();
    void			updateAuxData();

    void			showPosAttrib(int attr,bool yn);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    virtual bool		canBeRemoved()const;
private:
				~HorizonDisplay();
    void			removeEMStuff();

    EM::PosID			findClosestNode(const Coord3&) const;
    void			createDisplayDataPacks(
					int attrib,const DataPointSet*);

    void			removeSectionDisplay(const EM::SectionID&);
    visBase::VisualObject*	createSection(const EM::SectionID&) const;
    bool			addSection(const EM::SectionID&,TaskRunner*);
    void			emChangeCB(CallBacker*);
    int				getChannelIndex(const char* nm) const;
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

    void			calculateLockedPoints();
    void			initSelectionDisplay(bool erase);
    virtual void		updateSelections();
    void			updateLockedPointsColor();
    void			handleEmChange(const EM::ObjectCallbackData&);
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
	int				objid_;
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
				   int& objidx,int objid) const;
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

    TypeSet<ConstRefMan<ColTab::Mapper> > coltabmappers_;
    TypeSet<ConstRefMan<ColTab::Sequence> > coltabsequences_;
    bool				enabletextureinterp_;

    bool				usestexture_;
    char				resolution_;
    int					curtextureidx_;

    ObjectSet<Attrib::SelSpecList>	as_;
    ObjectSet<TypeSet<DataPack::ID> >	dispdatapackids_;
    BoolTypeSet				enabled_;
    TypeSet<int>			curshiftidx_;
    ObjectSet< TypeSet<float> >		shifts_;
    bool				validtexture_;
    bool				displayintersectionlines_;
    bool				displaysurfacegrid_;
    bool				showlock_;
    Threads::Lock			updatelock_;

    TypeSet<EM::SectionID>		oldsectionids_;
    TypeSet<StepInterval<int> >		olddisplayedrowranges_;
    TypeSet<StepInterval<int> >		olddisplayedcolranges_;

    ObjectSet<visBase::HorizonTextureHandler> oldhortexhandlers_;

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

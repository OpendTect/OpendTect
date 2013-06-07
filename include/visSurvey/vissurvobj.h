#ifndef vissurvobj_h
#define vissurvobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "color.h"
#include "cubesampling.h"
#include "datapack.h"
#include "gendefs.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "survinfo.h"
#include "vissurvscene.h"

class SurveyInfo;
class BaseMap;
class BaseMapObject;
class DataPointSet;
class IOPar;
class LineStyle;
class NotifierAccess;
class SeisTrcBuf;
class ZAxisTransform;
class InlCrlSystem;
class TaskRunner;

namespace ColTab  { class MapperSetup; class Sequence; }

namespace visBase
{
    class Transformation;
    class EventInfo;
    class TextureChannel2RGBA;
};

namespace Attrib  { class SelSpec; class DataCubes; }

namespace visSurvey
{

/*!\brief Base class for all 'Display' objects
*/

mClass SurveyObject
{
public:
			    
    virtual void		setInlCrlSystem(const SurveyInfo& si);
    				/*!<Don't use in new code. */
    const SurveyInfo*		getInlCrlSystem() const { return &SI(); }
    				/*!<Don't use in new code. */
    
    virtual const char*		getInlCrlSystemName() const;
    
    void			setInlCrlSystem(const InlCrlSystem&);
    const InlCrlSystem*		inlCrlSystem() const;

    virtual void		setBaseMap(BaseMap*);
    virtual Coord3		getNormal(const Coord3& pos) const
				{ return Coord3::udf(); }
    				/*!<Position and Normal are both in
				    displayspace. */
    virtual float		calcDist(const Coord3& pos) const
				{ return mUdf(float); }
    				/*<\Calculates distance between pick and 
				    object.
				    \note The distance is in display space.
				    \param pos Position to be checked in
				     	   displayspace.
				 \ */
    virtual float		maxDist() const		{ return sDefMaxDist();}
    				/*<\Returns maximum allowed distance between 
				    pick and object. If calcDist() > maxDist()
				    pick will not be displayed. */
    virtual bool		allowsPicks() const	{ return false; }
    				/*<\Returns whether picks can be created 
				    on object. */
    virtual bool		isPicking() const 	{ return false; }
    				/*<\Returns true if object is in a mode
				    where clicking on other objects are
				    handled by object itself, and not passed
				    on to selection manager .*/
    virtual void		getPickingMessage( BufferString& msg ) const
    				{ msg = "Picking"; }

    virtual void		snapToTracePos(Coord3&)	const {}
    				//<\Snaps coordinate to a trace position

    virtual NotifierAccess*	getMovementNotifier()	{ return 0; }
    				/*!<Gives access to a notifier that is triggered
				    when object is moved or modified. */

    virtual void		otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj ) {}
    				/*!< If other objects are moved, removed or
				     added in the scene, this function is
				     called. \note that it only notifies on
				     objects that return something on
				     getMovementNotifier().
				     \param whichobj refers to id of the
				     moved object */

    virtual void		setTranslation(const Coord3&) {}
    virtual Coord3		getTranslation() const
    				{ return Coord3(0,0,0); }

    virtual void		getChildren( TypeSet<int>& ) const	{}

    virtual bool		canDuplicate() const	{ return false;}
    virtual SurveyObject*	duplicate(TaskRunner*) const	{ return 0; }

    virtual MultiID		getMultiID() const	{ return MultiID(-1); }

    virtual void		showManipulator(bool yn)	{}
    virtual bool		isManipulatorShown() const	{ return false;}
    virtual bool		isManipulated() const		{ return false;}
    virtual bool		canResetManipulation() const	{ return false;}
    virtual void		resetManipulation()		{}
    virtual void		acceptManipulation()		{}
    virtual BufferString	getManipulationString() const	{ return ""; }
    virtual NotifierAccess*	getManipulationNotifier()	{ return 0; }

    virtual bool		allowMaterialEdit() const	{ return false;}
    				/*!\note Modification of color should be done
				  	 with setMaterial on
					 visBase::VisualObject */

    virtual const LineStyle*	lineStyle() const { return 0; }
    				/*!<If the linestyle can be set, a non-zero
				    pointer should be return. */
    virtual void		setLineStyle(const LineStyle&) {}
    virtual void		getLineWidthBounds(int& min,int& max);
    virtual bool		hasSpecificLineColor() const { return false; }
    				/*!<Specifies wether setLineStyle takes
				    regard to the color of the linestyle. */

    virtual bool		hasColor() const		{ return false;}
    virtual void		setColor(Color)			{}
    virtual Color		getColor() const		
    				{ return Color::DgbColor(); }

    virtual void		setAnnotColor(Color)			{}
    virtual Color		getAnnotColor() const		
    				{ return Color::DgbColor(); }

    virtual int			nrResolutions() const		{ return 1; }
    virtual BufferString	getResolutionName(int) const;
    virtual int			getResolution() const		{ return 0; }
    virtual void		setResolution(int,TaskRunner*)	{}

    virtual visBase::TextureChannel2RGBA* getChannels2RGBA()	{ return 0; }
    virtual bool		setChannels2RGBA(visBase::TextureChannel2RGBA*)
				{ return false; }

    enum AttribFormat		{ None, Cube, Traces, RandomPos, OtherFormat };
    				/*!\enum AttribFormat
					 Specifies how the object wants it's
					 attrib data delivered.
				   \var None
				   	This object does not handle attribdata.
				   \var	Cube
				   	This object wants attribdata as 
					DataCubes.
				   \var	Traces
				   	This object wants a set of traces.
    				   \var RandomPos
				        This object wants a table with 
					array positions.
    				   \var OtherFormat
				   	This object wants attribdata of a
					different kind. */

    virtual AttribFormat	getAttributeFormat(int attrib=-1) const;
    virtual bool		canHaveMultipleAttribs() const { return false; }
    virtual int			nrAttribs() const;
    virtual bool		canAddAttrib(int nrattribstoadd=1) const;
    virtual bool		addAttrib()		   { return false; }
    virtual bool		canRemoveAttrib() const;
    virtual bool		removeAttrib(int attrib)   { return false; }
    virtual bool		swapAttribs(int a0,int a1) { return false; }
    virtual void		setAttribTransparency(int,unsigned char) {}	
    virtual unsigned char	getAttribTransparency(int) const { return 0; }
    virtual const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
	    						   int version=0) const;
    virtual void		setColTabMapperSetup(int,
				     const ColTab::MapperSetup&,TaskRunner*);
    virtual const ColTab::Sequence* getColTabSequence(int) const { return 0; }
    virtual bool		canSetColTabSequence() const	{ return false;}
    virtual void		setColTabSequence(int,const ColTab::Sequence&,
	    					  TaskRunner*);
    virtual bool		canHandleColTabSeqTrans(int) const;
    
    virtual void		enableTextureInterpolation(bool)	{}
    virtual bool		textureInterpolationEnabled() const 
    				{ return true; }
    virtual bool		canEnableTextureInterpolation() const
    				{ return false; }

    virtual bool 		isAngle(int attrib) const	 {return false;}
    virtual void		setAngleFlag(int attrib,bool yn)	{}
    virtual void		enableAttrib(int attrib,bool yn)	{}
    virtual bool		isAttribEnabled(int attrib) const {return true;}
    virtual Pol2D3D		getAllowedDataType() const	{return Only3D;}
    
    virtual const TypeSet<float>* getHistogram(int attrib) const { return 0; }

    virtual void		removeSelection(const Selector<Coord3>&,
	    					TaskRunner*) {}
    virtual bool		canRemoveSelection() const	{ return false;}

    virtual void		   setSelSpec(int,const Attrib::SelSpec&){}
    virtual const Attrib::SelSpec* getSelSpec(int attrib) const  { return 0; }


    virtual bool		canHaveMultipleTextures() const { return false;}
    virtual int			nrTextures(int attrib) const	{ return 0; }
    virtual void		selectTexture(int attrib,int texture) {}
    virtual int			selectedTexture(int attrib) const { return 0; }
    virtual void		allowShading(bool)		{}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    Coord3& xyzpos,
					    BufferString& val,
					    BufferString& info) const
				{ val = mUdf(float); info = ""; }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
	    					IOPar&) const	{}
    virtual const MouseCursor*	getMouseCursor() const		{ return 0; }

    				/*!<Returns a mouse cursor that will
				    be used if this object under the
				    mouse in Act mode. */
				    
    virtual void		getObjectInfo(BufferString&) const	{}

    				// Data via DataPacks
    virtual bool		setDataPackID(int attrib,DataPack::ID,
	    				      TaskRunner*)
    				{ return false; }
    virtual DataPack::ID	getDataPackID(int attrib) const { return -1; }
    virtual DataPackMgr::ID	getDataPackMgrID() const	{ return -1; }
    virtual void		createAndDispDataPack(int, const DataPointSet*,
	    					      TaskRunner*){}
   
   				//Volume data 
    virtual CubeSampling	getCubeSampling( int attrib ) const
				{ CubeSampling cs; return cs; }
    				/*!<\returns the volume in world survey
				     coordinates. */
    virtual bool		setDataVolume(int attrib,
	    				      const Attrib::DataCubes*,
					      TaskRunner*)
				{ return false; }
    virtual const Attrib::DataCubes* getCacheVolume(int attr) const {return 0;}

    				//Trace-data
    virtual void		getDataTraceBids(TypeSet<BinID>&) const	{}
    virtual Interval<float>	getDataTraceRange() const
    				{ return Interval<float>(0,0); }
    virtual void		setTraceData(int attrib,SeisTrcBuf&,
	    				     TaskRunner*);
				// Random pos
				/*!< Every position is put in the DataPointSet
				  no matter which original patch it belongs to*/
    virtual void		getRandomPos(DataPointSet&,TaskRunner*) const {}
    virtual void		getRandomPosCache(int attrib,
	    					  DataPointSet&) const	{}
    virtual void		setRandomPosData( int attrib,
	    					  const DataPointSet*,
						  TaskRunner*)	{}
    virtual void		readAuxData()				{}

    virtual void		setScene(Scene* scn);
    virtual const Scene*	getScene() const	{ return scene_; }
    virtual Scene*		getScene()		{ return scene_; }

    virtual bool		setZAxisTransform(ZAxisTransform*,TaskRunner*)
				{return false;}
    virtual const ZAxisTransform* getZAxisTransform() const	 {return 0;}
    virtual bool		alreadyTransformed(int attrib) const;

    virtual void		lock( bool yn )		{ locked_ = yn; }
    virtual bool		isLocked() const	{ return locked_; }
    virtual NotifierAccess*	getLockNotifier()	{ return 0; }
    void	 		fillSOPar(IOPar&,TypeSet<int>&) const;
    int				useSOPar(const IOPar&);

    //TODO: as for now: vertical viewer is the only one available,
    //later on: allow timeslices and horizons with horizontal viewer
    virtual bool		canBDispOn2DViewer() const	{ return false;}
    virtual bool		isVerticalPlane() const		{ return true;}
    virtual bool		isInlCrl() const	    	{ return false;}
    
    static float		sDefMaxDist();
	
				//Old
    static const char*		sKeyColTabID()	{ return "Colortable ID"; }

    				//Current
    static const char*		sKeySequence()	{ return "Sequence"; }
    static const char*		sKeyMapper()	{ return "Mapper"; }
    static const char*		sKeyTextTrans()	{ return "Trans"; }
    static const char*		sKeyTC2RGBA()	{ return "TC2RGBA"; }
    static const char*		sKeyNrAttribs() { return "Nr Attribs"; }
    static const char*		sKeyAttribs()	{ return "Attrib "; }
    static const char*          sKeyLocked()    { return "Locked"; }
    static const char*		sKeySurvey()	{ return "Survey"; }

    void			setUserRefs( int attrib, BufferStringSet* nms )
				{ userrefs_.replace( attrib, nms ); }

protected:
    				SurveyObject();
				~SurveyObject();	
    static int			cValNameOffset()	{ return 12; }

    mutable BufferString	errmsg_;
    Scene*			scene_;
    bool			locked_;
    ObjectSet<BufferStringSet>	userrefs_;

    virtual BaseMapObject*	createBaseMapObject()	{ return 0; }
    BaseMapObject*		basemapobj_;

    const SurveyInfo*		survinfo_;
    BufferString		survname_; //Only from IOPar
};


}; // namespace visSurvey


/*!\page visSurvey 3D Visualisation - OpendTect specific

  This module contains front-end classes for displaying 3D objects. Most 
  functions in these classes deal with the geometry or position of the object, 
  as well as handling new data and information about the attribute 
  displayed.



*/


#endif


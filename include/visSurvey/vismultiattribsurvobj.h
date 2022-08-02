#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"

namespace visBase
{
    class TextureChannels;
    class TextureChannel2RGBA;
};

namespace ColTab  { class MapperSetup; class Sequence; }

namespace visSurvey
{

/*!Base class for objects with multitextures. Class handles all texture handling
   for inheriting classes, which avoids code duplication.
*/


mExpClass(visSurvey) MultiTextureSurveyObject : public visBase::VisualObjectImpl
    					      , public SurveyObject
{
public:
    bool			turnOn(bool yn);
    bool			isOn() const;
    bool			isShown() const;
    				//!<Returns true if displayed, i.e. it is
				//!<on, and has at least one enabled attribute.

    virtual int			nrResolutions() const			= 0;
    virtual void		setResolution(int,TaskRunner*)		= 0;
    int				getResolution() const;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    visBase::TextureChannel2RGBA* getChannels2RGBA();
    visBase::TextureChannels*	getChannels() const { return channels_; }

    bool			canHaveMultipleAttribs() const;
    bool			canAddAttrib(int nrattribstoadd=1) const;
    bool			canRemoveAttrib() const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;
    virtual void		allowShading(bool);

    const Attrib::SelSpec*	getSelSpec(int attrib,int version=0) const;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const;

    void			setSelSpec(int,const Attrib::SelSpec&);
    void			setSelSpecs(int attrib,
					    const TypeSet<Attrib::SelSpec>&);
    void			clearTextures();
    				/*!<Blanks all textures. */

    void			enableTextureInterpolation(bool);
    bool			textureInterpolationEnabled() const;

    bool 			isAngle(int attrib) const;
    void			setAngleFlag(int attrib,bool yn);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    const TypeSet<float>*	getHistogram(int) const;
    int				getColTabID(int) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
	    					     int version) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int) const;
    void			setColTabMapperSetup(int,
	    				const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int,const ColTab::Sequence&,
	    					  TaskRunner*);

    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures(int attrib) const;
    void			selectTexture(int attrib, int texture );
    int				selectedTexture(int attrib) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    virtual bool		canBDispOn2DViewer() const	{ return true; }
    bool			canEnableTextureInterpolation() const
				{ return channels_; }
    bool			canDisplayInteractively(
						Pos::GeomID geomid) const;
    bool			canDisplayInteractively() const;

    const char*			errMsg() const { return errmsg_.str(); }

protected:

    				MultiTextureSurveyObject();
				~MultiTextureSurveyObject();
    void			getValueString(const Coord3&,
	    				       BufferString&) const;
    				//!<Coord is in ztransformed space
    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const = 0;
    				//!<Coord is in attribute space

    void			updateMainSwitch();
    virtual void		addCache()				= 0;
    virtual void		removeCache(int)			= 0;
    virtual void		swapCache(int,int)			= 0;
    virtual void		emptyCache(int)				= 0;
    virtual bool		hasCache(int) const			= 0;
    virtual bool		init();

    visBase::TextureChannels*	channels_;

    int				resolution_;

private:
    ObjectSet<TypeSet<Attrib::SelSpec> > as_;
    bool			enabletextureinterp_;
    bool			onoffstatus_;

    static const char*		sKeySequence();
    static const char*		sKeyMapper();
    static const char*		sKeyResolution();
    static const char*		sKeyTextTrans();
};

} // namespace visSurvey


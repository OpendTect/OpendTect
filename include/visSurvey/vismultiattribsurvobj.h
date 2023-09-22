#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    bool			turnOn(bool yn) override;
    bool			isOn() const override;
    bool			isShown() const;
				//!<Returns true if displayed, i.e. it is
				//!<on, and has at least one enabled attribute.

    int				nrResolutions() const override		= 0;
    void			setResolution(int,TaskRunner*) override	= 0;
    int				getResolution() const override;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*)
								      override;
    visBase::TextureChannel2RGBA* getChannels2RGBA() override;
    visBase::TextureChannels*	getChannels() const override {return channels_;}

    bool			canHaveMultipleAttribs() const override;
    bool			canAddAttrib(int nrattribstoadd=1)
								const override;
    bool			canRemoveAttrib() const override;
    int				nrAttribs() const override;
    bool			addAttrib() override;
    bool			removeAttrib(int attrib) override;
    bool			swapAttribs(int attrib0,int attrib1) override;
    void			setAttribTransparency(int,
						      unsigned char) override;
    unsigned char		getAttribTransparency(int) const override;
    void			allowShading(bool) override;

    const Attrib::SelSpec*	getSelSpec(int attrib,
					   int version=-1) const override;
				//!< version=-1 gives current version

    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const override;

    void			setSelSpec(int,const Attrib::SelSpec&) override;
    void			setSelSpecs(int attrib,
				    const TypeSet<Attrib::SelSpec>&) override;
    void			clearTextures();
				/*!<Blanks all textures. */

    void			enableTextureInterpolation(bool) override;
    bool			textureInterpolationEnabled() const override;

    bool			isAngle(int attrib) const override;
    void			setAngleFlag(int attrib,bool yn) override;
    void			enableAttrib(int attrib,bool yn) override;
    bool			isAttribEnabled(int attrib) const override;
    const TypeSet<float>*	getHistogram(int) const override;
    int				getColTabID(int) const;

    const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
						 int version) const override;
    const ColTab::MapperSetup*	getColTabMapperSetup(int) const;
    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,
					TaskRunner*) override;
    const ColTab::Sequence*	getColTabSequence(int) const override;
    bool			canSetColTabSequence() const override;
    void			setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*) override;

    bool			canHaveMultipleTextures() const override
				{ return true; }
    int				nrTextures(int attrib) const override;
    void			selectTexture(int attrib,int texture) override;
    int				selectedTexture(int attrib) const override;

    virtual bool		hasCache(int) const			= 0;
    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const = 0;
				//!<Coord is in attribute space

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    bool			canBDispOn2DViewer() const override
				{ return true; }
    bool			canEnableTextureInterpolation() const override
				{ return channels_; }
    bool			canDisplayInteractively(
						Pos::GeomID geomid) const;
    bool			canDisplayInteractively() const;

    const char*			errMsg() const override { return errmsg_.str();}

protected:

				MultiTextureSurveyObject();
				~MultiTextureSurveyObject();
    void			getValueString(const Coord3&,
					       BufferString&) const;
				//!<Coord is in ztransformed space

    void			updateMainSwitch();
    virtual void		addCache()				= 0;
    virtual void		removeCache(int)			= 0;
    virtual void		swapCache(int,int)			= 0;
    virtual void		emptyCache(int)				= 0;
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

#ifndef vismultiattribsurvobj_h
#define vismultiattribsurvobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"

namespace visBase
{
    class MultiTexture2;
    class TextureChannels;
    class TextureChannel2RGBA;
};

namespace ColTab  { class MapperSetup; class Sequence; }

namespace visSurvey
{

/*!Base class for objects with multitextures. Class handles all texture handling
   for inheriting classes, which avoids code duplication.
*/


mClass MultiTextureSurveyObject : public visBase::VisualObjectImpl,
				  public SurveyObject
{
public:
    void			turnOn(bool yn);
    bool			isOn() const;
    bool			isShown() const;
    				//!<Returns true if displayed, i.e. it is
				//!<on, and has at least one enabled attribute.

    virtual int			nrResolutions() const			= 0;
    virtual void		setResolution(int,TaskRunner*)		= 0;
    int				getResolution() const;

    bool			setChannels2RGBA(visBase::TextureChannel2RGBA*);
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

    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
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

    void			fillPar(IOPar&, TypeSet<int>&) const;
    int				usePar(const IOPar&);
    virtual bool                canBDispOn2DViewer() const	{ return true; }
    bool			canEnableTextureInterpolation() const
				{ return channels_; }

    const char*			errMsg() const { return errmsg_.str(); }
    
    static const char*		sKeyTC2RGBA();
protected:

    				MultiTextureSurveyObject(
					bool usechannels );
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
    virtual bool		_init();

    visBase::MultiTexture2*	texture_;
    visBase::TextureChannels*	channels_;

    int				resolution_;

private:
    ObjectSet<Attrib::SelSpec>	as_;
    bool			enabletextureinterp_;
    bool			onoffstatus_;

    static const char*		sKeySequence();
    static const char*		sKeyMapper();
    static const char*		sKeyResolution();
    static const char*		sKeyTextTrans();
};

} // namespace visSurvey

#endif

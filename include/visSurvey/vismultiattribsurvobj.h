#ifndef vismultiattribsurvobj_h
#define vismultiattribsurvobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismultiattribsurvobj.h,v 1.10 2007-08-31 12:48:58 cvskris Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"

namespace visBase { class MultiTexture2; }

namespace visSurvey
{

/*!Base class for objects with multitextures. Class handles all texture handling
   for inheriting classes, which avoids code duplication.
*/


class MultiTextureSurveyObject : public visBase::VisualObjectImpl,
				 public SurveyObject
{
public:
    void			turnOn(bool yn);
    bool			isOn() const;

    virtual int			nrResolutions() const			= 0;
    virtual void		setResolution(int)			= 0;
    int				getResolution() const;

    bool			canHaveMultipleAttribs() const;
    bool			canAddAttrib() const;
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
    bool 			isClassification(int attrib) const;
    void			setClassification(int attrib,bool yn);
    bool 			isAngle(int attrib) const;
    void			setAngleFlag(int attrib,bool yn);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    const TypeSet<float>*	getHistogram(int) const;
    int				getColTabID(int) const;

    bool			canHaveMultipleTextures() const { return true; }
    int				nrTextures(int attrib) const;
    void			selectTexture(int attrib, int texture );
    int				selectedTexture(int attrib) const;

    void			fillPar(IOPar&, TypeSet<int>&) const;
    int				usePar(const IOPar&);
    virtual bool                canBDispOn2DViewer() const	{ return true; }
    
protected:
    				MultiTextureSurveyObject();
				~MultiTextureSurveyObject();
    void			getValueString(const Coord3&,
	    				       BufferString&) const;

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const = 0;

    void			updateMainSwitch();
    virtual void		addCache()				= 0;
    virtual void		removeCache(int)			= 0;
    virtual void		swapCache(int,int)			= 0;
    virtual void		emptyCache(int)				= 0;
    virtual bool		hasCache(int) const			= 0;
    virtual bool		_init();

    visBase::MultiTexture2*	texture_;
    int				resolution_;

private:
    ObjectSet<Attrib::SelSpec>	as_;
    BoolTypeSet			isclassification_;
    bool			onoffstatus_;

    static const char*		sKeyResolution();
    static const char*		sKeyTextTrans();
};

}; // Namespace visSurvey


/*! \mainpage 3D Visualisation - OpendTect specific

  This module contains front-end classes for displaying 3D objects. Most 
  functions in these classes deal with the geometry or position of the object, 
  as well as handling new data and information about the attribute 
  displayed.

*/


#endif

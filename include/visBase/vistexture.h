#ifndef vistexture_h
#define vistexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture.h,v 1.11 2003-05-28 09:46:40 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class DataClipper;
class BasicTask;
class Color;
class IOPar;
class LinScaler;
class SoComplexity;
class SoGroup;
class SoSwitch;
class visBaseTextureColorIndexMaker;

namespace visBase
{

class VisColorTab;
class ThreadWorker;

/*!\brief
is a base class for Texture2 and Texture3 and should not be used directly.

A number of caches are implemented to minimize the calculation-time
when the colortable is updated.

If ThreadWorker is set, it utilizes mt processing.

*/

class Texture : public SceneObject
{
public:
    enum		DataType { Color, Transparency,
				   Hue, Saturation, Brightness };
    bool		turnOn( bool yn );
    bool		isOn()	const;
    			
    void		setAutoScale( bool yn );
    bool		autoScale() const;

    void		setColorTab( VisColorTab& );
    VisColorTab&	getColorTab();

    void		setClipRate( float );
    			/*!< Relayed to colortable */
    float		clipRate() const;
    			/*!< Relayed to colortable */

    void		setResolution(int res)		{ resolution = res; }
    int			getResolution() const		{ return resolution; }

    const TypeSet<float>& getHistogram() const;

    void		setUseTransperancy(bool yn);
    bool		usesTransperancy() const;

    void		setThreadWorker(ThreadWorker*);
    ThreadWorker*	getThreadWorker();

    void		setTextureQuality(float);
    float		getTextureQuality() const;

    SoNode*		getData();

    virtual void        fillPar(IOPar&,TypeSet<int>&) const;
    virtual int         usePar(const IOPar&);

protected:
    			Texture();
    			~Texture();
    void		setResizedData( float*, int sz, DataType );
    			/*!< Is taken over by me */
    int			nextPower2(int,int,int) const;

    virtual unsigned char* getTexturePtr()				= 0;
    virtual void	finishEditing()					= 0;

    SoSwitch*		onoff;
    SoGroup*		texturegrp;
    SoComplexity*	quality;

    int			resolution;

private:
    void		colorTabChCB( CallBacker* );
    void		colorSeqChCB( CallBacker* );
    void		autoscaleChCB( CallBacker* );

    void		clipData();
    void		makeColorIndexes();
    void		makeTexture();
    void		makeColorTables();

    ObjectSet<float>	datacache;
    unsigned char*	indexcache;
    int			cachesize;

    TypeSet<LinScaler>	datascales;
			/*!<\note The first entry is not used since
				  the color range is in the coltab
			*/

    ::Color*		colortabcolors;

    unsigned char*	red;
    unsigned char*	green;
    unsigned char*	blue;
    unsigned char*	trans;

    TypeSet<float>	histogram;

    bool		usetrans;

    VisColorTab*	colortab;
    ObjectSet<visBaseTextureColorIndexMaker> colorindexers;
    ObjectSet<BasicTask> texturemakers;
    ThreadWorker*	threadworker;

    static const char*	colortabstr;
    static const char*	texturequalitystr;
    static const char*	usestexturestr;
    static const char*	resolutionstr;
};

};

#endif

#ifndef vistexture_h
#define vistexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture.h,v 1.8 2003-02-19 15:34:09 nanne Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class DataClipper;
class BasicTask;
class visBaseTextureColorIndexMaker;
class SoSwitch;
class SoGroup;
class SoComplexity;

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

    void		setThreadWorker( ThreadWorker* );
    ThreadWorker*	getThreadWorker();

    void		setTextureQuality(float);
    float		getTextureQuality() const;

    SoNode*		getData();

protected:
    			Texture();
    			~Texture();
    void		setResizedData( float*, int sz );
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

    float*		datacache;
    unsigned char*	indexcache;
    int			cachesize;

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
};

};

#endif

#ifndef vistexture_h
#define vistexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture.h,v 1.3 2003-01-23 11:58:23 nanne Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class DataClipper;
class BasicTask;
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
    			
    void		setAutoScale( bool yn );
    bool		autoScale() const;

    void		setColorTab( VisColorTab& );
    VisColorTab&	getColorTab();

    void		setClipRate( float );
    float		clipRate() const;

    const TypeSet<float>& getHistogram() const;

    void		setUseTransperancy(bool yn);
    bool		usesTransperancy() const;

    void		setThreadWorker( ThreadWorker* );
    ThreadWorker*	getThreadWorker();


protected:
    			Texture();
    			~Texture();
    void		setResizedData( float*, int sz );
    			/*!< Is taken over by me */

    virtual unsigned char* getTexturePtr()				= 0;
    virtual void	finishEditing()					= 0;

private:
    void		colorTabChCB( CallBacker* );
    void		colorSeqChCB( CallBacker* );

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

    bool		autoscale;
    bool		usetrans;

    VisColorTab*	colortab;
    DataClipper&	dataclipper;
    ObjectSet<visBaseTextureColorIndexMaker> colorindexers;
    ObjectSet<BasicTask> texturemakers;
    ThreadWorker*	threadworker;
};

};

#endif

#ifndef vismultitexture2_h
#define vismultitexture2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Dec 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vismultitexture.h"
#include "rowcol.h"

class SoSwitch;
class SoGroup;
class SoColTabMultiTexture2;
class SoComplexity;
class SoShaderTexture2;
class SoShaderProgram;
class SoFragmentShader;
class SoShaderParameter1i;
class SoShaderParameter2i;
class SoShaderParameterArray1f;

namespace visBase
{

mClass MultiTexture2 : public MultiTexture
{
public:
    static MultiTexture2*	create()
    				mCreateDataObj(MultiTexture2);

    int				maxNrTextures() const;
    bool			turnOn(bool yn);
    bool			isOn() const;
    void			clearAll();
    				/*!<Sets all arrays to zero. Will cause
				    the texture to become white. After clearAll
				    is called, data of any size will be
				    accepted.*/

    bool			canUseShading() const;
    bool			usesShading() const   { return useshading_; }
    void			allowShading(bool yn) { allowshading_ = yn; }
    static int			getMaxTextureSize();
   
    bool			splitsTexture() const;
    void			splitTexture(bool yn); 
    const TypeSet<int>&		getUsedTextureUnits() 
    					{ return usedtextureunits_;}

    void			setTextureTransparency(int, unsigned char);
    unsigned char		getTextureTransparency(int) const;
    void			swapTextures(int,int);
    void			setOperation(int texture,Operation);
    Operation			getOperation(int texture) const;
    void			setTextureRenderQuality(float);
    float			getTextureRenderQuality() const;
    void			enableInterpolation(bool);
    bool			interpolationEnabled() const;
    bool			setData(int texture,int version,
	    				const Array2D<float>*, bool copy=false);
    				/*!<\param copy Specifies whether the data
				                should be copied or not. If it's
						not copied, it is assumed to
						remain in mem until this
						object is destroyed, or
						another data is set */
				
    bool			setDataOversample(int texture,int version,
	    				int resolution,bool interpol,
					const Array2D<float>*,bool copy=false);
    				/*!<\param copy Specifies whether the data
				                should be copied or not. If it's
						not copied, it is assumed to
						remain in mem until this
						object is destroyed, or
						another data is set */
    bool			setIndexData(int texture,int version,
					     const Array2D<unsigned char>*);

    RowCol			getSize() const		{ return size_; }

protected:

    			~MultiTexture2();
    void		polyInterp(const Array2D<float>&,
				   Array2D<float>&) const;
    void		nearestValInterp(const Array2D<float>&,
					 Array2D<float>&) const;
    bool		setSize(int,int);

    void		updateSoTextureInternal(int texture);
    void		insertTextureInternal(int texture);
    void		removeTextureInternal(int texture);

    void		updateColorTables();
    void		updateShadingVars();
    void		createShadingVars();
    void		createShadingProgram(int nrlayers,BufferString&) const;
    void		reviewShading();

    SoSwitch*			switch_; // off/noshading/shading 
    RowCol			size_;
    bool			allowshading_;	//user setting
    bool			useshading_;	//current status
    bool			dosplittexture_;  //for shading only
    TypeSet<int>		usedtextureunits_;
    bool			enableinterpolation_;

    //Non-shading
    SoGroup*			nonshadinggroup_;
    SoColTabMultiTexture2*	texture_;
    SoComplexity*		complexity_;

    //Shading
    SoGroup*			shadinggroup_;
    SoShaderTexture2*		ctabtexture_;
    SoGroup*			datatexturegrp_;
    SoShaderProgram*		shaderprogram_;
    SoFragmentShader*		fragmentshader_;
    SoShaderParameter1i*	numlayers_;
    SoShaderParameter1i*	startlayer_;
    SoShaderParameterArray1f*	layeropacity_;
    SoShaderParameter1i*	layersize0_;
    SoShaderParameter1i*	layersize1_;
    SoShaderParameter1i*	ctabunit_;
    TypeSet<float>		opacity_;
    SoComplexity*		shadingcomplexity_;
    static const char*		sVertexShaderProgram();

    virtual SoNode*		gtInvntrNode();

};

}; // Namespace

#endif

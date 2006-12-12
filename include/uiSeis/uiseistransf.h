#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.21 2006-12-12 11:16:57 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "seisioobjinfo.h"
class IOObj;
class Executor;
class uiGenInput;
class SeisSelData;
class uiSeisSubSel;
class SeisResampler;
class uiSeis2DSubSel;
class uiSeis3DSubSel;
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

    class Setup
    {
    public:
			Setup()
			    : geom_(Seis::Vol)
			    , withstep_(true) //!< Can user specify steps?
			    , multi2dlines_(false) //!< Allow 'all' lines (2D)
			    , fornewentry_(true) //!< New line or existing (2D)
			    			{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,fornewentry)
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,multi2dlines)
    };

			uiSeisTransfer(uiParent*,const Setup&);
    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
	    			   const char* executor_txt,
				   const char* work_txt,
				   const char* attr2dnm,
				   const char* linenm2d_overrule=0) const;

    uiSeisSubSel*	selfld;
    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    uiSeis2DSubSel*	selFld2D(); //!< null when not 2D
    uiSeis3DSubSel*	selFld3D(); //!< null when not 3D

    void		setSteering(bool);
    void		setInput(const IOObj&);
    void		getSelData(SeisSelData&) const;
    SeisResampler*	getResampler() const; //!< may return null

    int			maxBytesPerSample() const;
    SeisIOObjInfo::SpaceInfo spaceInfo() const;

    bool		removeNull() const;

protected:

    Setup		setup_;

};


#endif

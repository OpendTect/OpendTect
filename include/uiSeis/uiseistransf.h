#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.20 2005-08-15 16:17:07 cvsbert Exp $
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
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

    class Setup
    {
    public:
			Setup()
			    : withformat_(true)	//!< Can user select storage?
			    , withstep_(true) //!< Can user specify steps?
			    , multi2dlines_(false) //!< Allow 'all' lines (2D)
			    , fornewentry_(true) //!< New line or existing (2D)
			    , prestack_(false) //!< Pre-Stack seismics?
			    			{}

	mDefSetupMemb(bool,withformat)
	mDefSetupMemb(bool,fornewentry)
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,multi2dlines)
	mDefSetupMemb(bool,prestack)
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

    void		setInput(const IOObj&);
    bool		is2D() const		{ return is2d; }
    void		set2D(bool);
    bool		isSteer() const		{ return issteer; }
    void		setSteering(bool);
    bool		isPS() const		{ return isps; }
    void		getSelData(SeisSelData&) const;
    SeisResampler*	getResampler() const; //!< may return null

    int			maxBytesPerSample() const;
    SeisIOObjInfo::SpaceInfo spaceInfo() const;

    bool		removeNull() const;

protected:

    bool		is2d;
    bool		isps;
    bool		issteer;

    void		updFldsForType(CallBacker*);

};


#endif

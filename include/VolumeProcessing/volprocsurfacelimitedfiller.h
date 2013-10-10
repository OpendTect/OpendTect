#ifndef volprocsurfacelimitedfiller_h
#define volprocsurfacelimitedfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "multiid.h"
#include "volprocchain.h"

namespace Geometry { class FaultStickSurface; }
namespace EM { class Fault; class Horizon; class Horizon3D; }

namespace VolProc
{
/*!Fills a volume with values. The borders are given by a set of horizons
   or faults with pre set side to calculate. The values are calculated based on
   v0+g*d, where for each variable we use either fixed values or from some 
   fixed horizon reference data. */

    
mExpClass(VolumeProcessing) SurfaceLimitedFiller : public Step
{
public:
    		mDefaultFactoryCreatorImpl( Step, SurfaceLimitedFiller );
		mDefaultFactoryInstanciationBase( "SurfaceLimitedFiller",
						  "Surface limited filler" );
    
    			~SurfaceLimitedFiller();
			SurfaceLimitedFiller();

    bool		isOK() const;

    bool		needsInput() const		{ return false; }
    
    bool		setSurfaces(const TypeSet<MultiID>&,
	    			    const TypeSet<char>& fillside);
    			/*Assume going down increases z. 
			  For horizons, side = 1 if go below, -1 if go above.*/
   
    int			nrOfSurfaces() const		 {return side_.size();}
    char		getSurfaceFillSide(int idx) const{ return side_[idx]; }
    const MultiID*	getSurfaceID(int idx) const;

    			//Start value/grid stuff
    bool		usesStartValue() const	    { return usestartval_; }
    void		useStartValue(bool yn)	    { usestartval_ = yn; } 
    
    float		getStartValue() const	    { return fixedstartval_; }
    void		setStartValue(float vel)    { fixedstartval_ = vel; }
    
    bool		setStartValueHorizon(const MultiID*);
    const MultiID*	getStartValueHorizonID() const;
    int			getStartAuxdataIdx() const { return startauxdataidx_;}
    void		setStartAuxdataIdx(int i)  { startauxdataidx_ = i; }

    			//Gradient value/grid stuff
    bool		usesGradientValue() const   { return usegradient_; }
    void		useGradientValue(bool yn)   { usegradient_ = yn; }

    bool		isGradientVertical() const  { return gradvertical_; }
    void		setGradientVertical(bool yn){ gradvertical_ = yn; }
    
    float		getGradient() const	{ return fixedgradient_; }
    void		setGradient(float grd)	{ fixedgradient_ = grd; }
    
    bool		setGradientHorizon(const MultiID*);
    const MultiID*	getGradientHorizonID() const;
    int			getGradAuxdataIdx()	{ return gradauxdataidx_; }
    void		setGradAuxdataIdx(int i){ gradauxdataidx_ = i; }

    			//Reference horizon/z stuff 
    bool		usesRefZValue() const		{ return userefz_; }
    void		useRefZValue(bool yn)		{ userefz_ = yn; } 
				
    void		setRefZValue(float zv)		{ refz_ = zv; }
    float		getRefZValue() const		{ return refz_; }

    bool		setRefHorizon(const MultiID*);
    const MultiID*	getRefHorizonID() const;
    
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    bool		useHorInterFillerPar(const IOPar&);

    void		releaseData();
    bool		canInputAndOutputBeSame() const { return true; }
    bool		needsFullVolume() const         { return false; }   
    const char*		errMsg() const 			{return errmsg_.buf();} 

protected:
    
    bool		prepareComp(int);
    bool		computeBinID(const BinID&, int);
    EM::Horizon*	loadHorizon(const MultiID&) const;
    			//!<\note horizon is reffed on return.
    bool		prefersBinIDWise() const	{ return true; }

    static const char*	sKeySurfaceID()		{ return "Surface MID"; }
    static const char*	sKeySurfaceFillSide()	{ return "Surface fill side"; }
    static const char*	sKeyNrSurfaces()	{ return "Nr of surfaces"; }
    
    static const char*	sKeyUseStartValue() { return "Use start value"; }
    static const char*	sKeyStartValue()    { return "Start value"; }
    static const char*	sKeyStartValHorID() { return "Start value horizon"; }
    static const char*	sKeyStartAuxDataID(){ return "Start auxdata id"; }
    
    static const char*	sKeyUseGradValue()  { return "Use gradient value"; }
    static const char*	sKeyGradValue()	    { return "Gradient value"; }
    static const char*	sKeyGradHorID()	    { return "Gradient horizon"; }
    static const char*	sKeyGradAuxDataID() { return "Gradient auxdata id"; }
    static const char*	sKeyGradType()	    { return "Gradient type"; }
    
    static const char*	sKeyRefHorID()	    { return "Reference horizon"; }
    static const char*	sKeyRefZ()	    { return "Reference z"; }
    static const char*	sKeyUseRefZ()	    { return "Use reference z"; }

    MultiID		gradhormid_;
    EM::Horizon3D*	gradhorizon_;
    int			gradauxdataidx_;
    float		fixedgradient_;
    bool		usegradient_;
    bool		gradvertical_;

    MultiID		starthormid_;
    EM::Horizon3D*	starthorizon_;
    int			startauxdataidx_;
    float		fixedstartval_;
    bool		usestartval_;
    
    MultiID		refhormid_;
    EM::Horizon*	refhorizon_;
    float		refz_;
    bool		userefz_;
    BufferString	errmsg_;
  
    			/*The following four have the same size, for any idx,
			  faults_[idx] or hors_[idx] is 0. */
    TypeSet<char>	side_;
    TypeSet<MultiID>	surfacelist_;
    ObjectSet<EM::Horizon>			hors_;
    ObjectSet<Geometry::FaultStickSurface>	faults_;

    bool		usebottomval_;
    double		valrange_;
};

}; //namespace


#endif

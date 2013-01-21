#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "namedobj.h"

#include "enums.h"
#include "linekey.h"
#include "multiid.h"
#include "wellio.h"
#include <iosfwd>

class IOPar;

#define mIsUnvalidD2TM(wd) ( !wd.haveD2TModel() || wd.d2TModel()->size()<5 )
namespace WellTie
{

mExpClass(WellAttrib) Setup
{
public:
    			enum CorrType { None, Automatic, UserDefined };
			DeclareEnumUtils(CorrType)

			Setup()
			    : wellid_(-1)
			    , seisid_()
			    , wvltid_(-1)
			    , issonic_(true)
			    , linekey_(0)
			    , useexistingd2tm_(true) 
			    , corrtype_(Automatic)    
			    , is2d_(false)				 
			    {}


				Setup( const Setup& setup ) 
				    : wellid_(setup.wellid_)
				    , seisid_(setup.seisid_)
				    , wvltid_(setup.wvltid_)
				    , issonic_(setup.issonic_)
				    , seisnm_(setup.seisnm_)
				    , vellognm_(setup.vellognm_)
				    , denlognm_(setup.denlognm_)
				    , linekey_(setup.linekey_)
				    , is2d_(setup.is2d_)
				    , useexistingd2tm_(setup.useexistingd2tm_)
				    , corrtype_(setup.corrtype_) 
				    {}	
		
    MultiID			wellid_;
    MultiID        		seisid_;
    MultiID               	wvltid_;
    LineKey			linekey_;
    BufferString        	seisnm_;
    BufferString        	vellognm_;
    BufferString          	denlognm_;
    bool                	issonic_;
    bool                	is2d_;
    bool 			useexistingd2tm_;
    CorrType			corrtype_;
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static Setup&		defaults();
    static void                 commitDefaults();

    static const char* 	sKeyCSCorrType() 
    				{ return "CheckShot Corrections"; }
    static const char* 	sKeyUseExistingD2T() 
    				{ return "Use Existing Depth/Time model"; }
};


mExpClass(WellAttrib) IO : public Well::IO
{
public:
    				IO(const char* f,bool isrd)
				: Well::IO(f,isrd) {}

    static const char*  	sKeyWellTieSetup();
};


mExpClass(WellAttrib) Writer : public IO
{
public:
				Writer(const char* f)
				    : IO(f,false) {}

    bool			putWellTieSetup(const WellTie::Setup&) const;

    bool          	        putIOPar(const IOPar&,const char*) const; 
protected:

    bool          	        putIOPar(const IOPar&,const char*,
	    					std::ostream&) const; 
    bool                	wrHdr(std::ostream&,const char*) const;
};

mExpClass(WellAttrib) Reader : public IO
{
public:
				Reader(const char* f)
				    : IO(f,true) {}

    void			getWellTieSetup(WellTie::Setup&) const;

    IOPar* 			getIOPar(const char*) const;

protected:
    IOPar* 			getIOPar(const char*,std::istream&) const;	
};

}; //namespace WellTie
#endif


#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: welltiesetup.h,v 1.10 2009-11-11 13:34:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

#include "multiid.h"
#include "linekey.h"
#include "welltieunitfactors.h"

#include "wellio.h"
#include <iosfwd>

class IOPar;
class MultiID;

namespace WellTie
{

mClass Setup
{
public:
			Setup()
			    : wellid_(*new MultiID())
			    , seisid_(*new MultiID())
			    , wvltid_(*new MultiID())
			    , linekey_(*new LineKey())		     
			    , issonic_(true)
			    , isinitdlg_(true)
			    , corrvellognm_("CS Corrected ")
			    , unitfactors_(0)
			    {}


			Setup( const Setup& setup ) 
			    : wellid_(setup.wellid_)
			    , seisid_(setup.seisid_)
			    , wvltid_(setup.wvltid_)
			    , issonic_(setup.issonic_)
			    , isinitdlg_(setup.isinitdlg_)
			    , seisnm_(setup.seisnm_)
			    , vellognm_(setup.vellognm_)
			    , denlognm_(setup.denlognm_)
			    , corrvellognm_(setup.corrvellognm_)
			    , unitfactors_(setup.unitfactors_)
			    , linekey_(setup.linekey_)
			    {}	
	
    MultiID			wellid_;
    MultiID        		seisid_;
    MultiID               	wvltid_;
    LineKey			linekey_;
    BufferString        	seisnm_;
    BufferString        	vellognm_;
    BufferString          	denlognm_;
    BufferString          	corrvellognm_;
    bool                	issonic_;
    bool 			isinitdlg_;
    
    WellTie::UnitFactors 	unitfactors_;
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static Setup&		defaults();
    static void                 commitDefaults();

protected:
    
};


mClass IO : public Well::IO
{
public:
    				IO(const char* f,bool isrd)
				: Well::IO(f,isrd)
				{}

    static const char*  	sKeyWellTieSetup();
    static const char*  	sExtWellTieSetup();
};



mClass Writer : public IO
{
public:
				Writer(const char* f,const WellTie::Setup& s)
				    : IO(f,false)
				    , wts_(s)
				    {}

    bool          	        putWellTieSetup() const;  
    bool 			putWellTieSetup(std::ostream&) const;

protected:
   
    const WellTie::Setup& 	wts_;
    bool                	wrHdr(std::ostream&,const char*) const;

};


mClass Reader : public IO
{
public:
				Reader(const char* f,WellTie::Setup& s)
				    : IO(f,true)
				    , wts_(s)
				    {}
  
    bool               		getWellTieSetup() const;	
    bool                	getWellTieSetup(std::istream&) const;


protected:
   
    WellTie::Setup& 	wts_;
};

}; //namespace WellTie
#endif

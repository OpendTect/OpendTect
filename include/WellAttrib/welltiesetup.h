#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: welltiesetup.h,v 1.17 2011-01-20 10:21:38 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

#include "linekey.h"
#include "multiid.h"
#include "wellio.h"
#include <iosfwd>

class IOPar;

#define mIsUnvalidD2TM(wd) ( !wd.haveD2TModel() || wd.d2TModel()->size()<5 )
namespace WellTie
{

mClass Setup
{
public:
			Setup()
			    : wellid_(-1)
			    , seisid_(-1)
			    , wvltid_(-1)
			    , issonic_(true)
			    , isinitdlg_(true)
			    , corrvellognm_("CS Corrected ")
			    , linekey_(0)	 
			    , useexistingd2tm_(true)  	 
			    , is2d_(false)				 
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
				    , linekey_(setup.linekey_)
				    , useexistingd2tm_(setup.useexistingd2tm_) 
				    , is2d_(setup.is2d_)
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
    bool                	is2d_;
    bool 			isinitdlg_;
    bool 			useexistingd2tm_;
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static Setup&		defaults();
    static void                 commitDefaults();
};


mClass IO : public Well::IO
{
public:
    				IO(const char* f,bool isrd)
				: Well::IO(f,isrd)
				{}

    static const char*  	sKeyWellTieSetup();
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

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinsthtmlcomposer.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "odinsthtmlcomposer.h"

#include "file.h"
#include "filepath.h"
#include "odinstpkgprops.h"
#include "odinstpkgselmgr.h"

static const char* empty_string = "";

#define addAnchor( command, extra  ) \
    add( "<a ").add( extra ).add(" href=\"").add( odprotocol ).add( command ) \
	.add("/").add( pp.pkgnm_).add( "\">")

#define addImage( file, w, h, extra ) \
    add( "<img src=\"" ).add( file ).add( "\" " ).add( extra ).add(" width=\"").add( w ) \
	.add( "\" height=\"").add( h ) \
	.add( "\" alt=\"image\">" )


ODInst::HtmlComposer::HtmlComposer()
{}


ODInst::HtmlComposer::~HtmlComposer()
{}


BufferString ODInst::HtmlComposer::generateHTML( const PkgProps& pp, 
						 PkgSelMgr* pkgselmgr )
{
    BufferString imagename = pp.pkgnm_;
    imagename += ".png";
    FilePath imagfp( ODInst::sTempDataDir() );
    imagfp.add( "odinst_images" );
    imagfp.add( imagename );

    BufferString html( "<html><body style=\"font-family:Arial;"
		       "font-size:12px;\">");
    
    const char* externalanchorend = "\" target=\"_blank\" align=\"left\">";
    html.add( "<table border=\"0\"><tr><td><h1>" );
    html.add( pp.usrname_ );
    html.add( "</h1>" );
    html.add( pp.desc_.cat() );
    html.add( "<br><br> <a href=\"").add( htmlprot ).add( "www." );
    html.add( pp.getURL() );
    html.add( externalanchorend ).add( "More info</a>" );
    
    if ( pp.commercial_ && !pp.isDoc() )
    {
	html.add( "<br><br> <a href=\"").add( htmlprot )
	    .add( "www.opendtect.org" )
	    .add( "/relman/scripts/request_evaluation_license.php" )
	    .add( "?plugin=" ).add( pp.pkgnm_ );
	html.add( externalanchorend ).add( "Request evaluation license</a>" );
    }

    if ( pkgselmgr )
    {
	html.add( "<p>" );
	generateActionHtml( pp, html, pkgselmgr );

	html.add( "<br><br>Creator: <A href=\"").add( htmlprot ).add( "www." ).add( pp.creator_->url_ )
	    .add( "\">").add( pp.creator_->name_ ).add("</a>");
	if ( pkgselmgr->isInstalled(pp) )
	    html.add( "<br>Installed version: " )
		.add( pkgselmgr->version(pp,true).userName() );
	html.add( "<br>Available version: " )
	    .add( pp.ver_.userName() );
	if ( pkgselmgr->isInstalled( pp ) )
	    html.add( "<br>" ).addAnchor( filelistcmd, empty_string ).add("File list</a>\n" );
	html.add( "<br><br>" ).addAnchor( closedetailcmd,  empty_string ).add( "Back</a>" );
    }

    html.add( "</td><td>").addImage(imagfp.fullPath(), 100, 100, empty_string )
	.add( "</td></tr></table>" ); 


    html.add( "</body> </html>" );
    html_ = html;
    return html_;
}


BufferString ODInst::HtmlComposer::generateMainSelHtml(
					ObjectSet<const ODInst::PkgProps>& pkgs,
					PkgSelMgr* pkgselmgr,
					const PkgGroupSet& grps )
{
    BufferString html( "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" );
    html.add( "<html>" );
    html.add( "<head><title></title>" );
    html.add( "<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">" );
    html.add( "<style type=\"text/css\">\n" )
	.add( "body.c3 {font-family:Arial; font-size:12px;}\n" )
	.add( "tr.c5 {  background-color: #DDDDDD; } \n " )
	.add( "tr.c6 {  background-color: #FFFFFF; } \n" )
	.add( "a.c1 {text-decoration:none;}\n" )
	.add( "</style>\n" );
    
    html.add( "</head>" );

    html.add( "<body class=\"c3\">\n" );
    html.add( "<table border=\"0\" cellspacing=\"0\" cellpadding=\"2\">\n" );

    int nrperrow = 3;
    int nrthisrow = 0;

    BufferString titles;
    BufferString mains;
    BufferString bottoms;
    int row = 0;
    for ( int idx=0; idx<pkgs.size(); idx++ )
    {
	generatePackageTableCell( *pkgs[idx], titles, mains, bottoms, 
				    pkgselmgr, grps );

	nrthisrow++;
	if ( nrthisrow==nrperrow || idx==pkgs.size()-1 )
	{
	    const char* bgcolor = (row%2) ? "c5" : "c6";
	    if ( titles.size() ) html.add( " <tr>\n" ).add( titles ).add( "\n </tr>" );
	    if ( mains.size() ) html.add( " <tr height=\"128\" class=\"").add( bgcolor ).add( "\">\n" ).add( mains ).add( "\n </tr>" );
	    if ( bottoms.size() ) html.add( " <tr>\n" ).add( bottoms ).add( "\n </tr>" );

	    titles.setEmpty();
	    mains.setEmpty();
	    bottoms.setEmpty();

	    nrthisrow = 0;
	    row++;
	}
    }

    html += "</table></body></html>";
    return html;
}


void ODInst::HtmlComposer::generatePackageTableCell( const ODInst::PkgProps& pp,
	BufferString& title, BufferString& main, BufferString& bottom,
	PkgSelMgr* pkgselmgr,const PkgGroupSet& pkggrps ) const
{
    BufferString imagename = pp.pkgnm_;
    imagename += ".png";

    FilePath imagfp( ODInst::sTempDataDir() );
    imagfp.add( "odinst_images" );
    imagfp.add( imagename );
    
#define mCutoff 20
    BufferString displayname = pp.usrname_;
    if ( displayname.size()>(mCutoff+4) )
    {
	displayname.buf()[mCutoff] = 0;
	displayname.insertAt( mCutoff, " ..." );
    }

    main.add( "\n  <td>" ).addAnchor( detailscmd, "class=\"c1\""  );
    main.addImage( imagfp.fullPath(), 100, 100, "" );
    main.add( "</a></td><td width=\"160\"><b>" );
    main.addAnchor( detailscmd, "class=\"c1\""  );
    main.add( displayname ).add("</a></b>" );

    BufferString shortdesc = pp.shortdesc_;
    if ( shortdesc.isEmpty() )
    {
    	for ( int igrp=0; igrp<pkggrps.size(); igrp ++ )
    	{
	    const ODInst::PkgGroup& pg = *pkggrps[igrp];
	    if ( pg.indexOf( &pp ) !=-1 )
	    {
	    	if ( shortdesc.size() )
		    shortdesc.add( ", " );

	    	shortdesc += pg.name_;
	    }
    	}
    }

    main.add( "<br>" ).add( shortdesc );
    main.add( "<br><br>" );
    generateActionHtml( pp, main, pkgselmgr );
    main.add( "</td>\n" );
}


void ODInst::HtmlComposer::generateActionHtml( const ODInst::PkgProps& pp,
						BufferString& html,
						PkgSelMgr* pkgselmgr ) const
{
    BufferString anchor;
    BufferString buttonimage;
    BufferString statusimage;

    if ( pkgselmgr->isSelected(pp) )
    {
	anchor.addAnchor( uninstallcmd, empty_string );

	if ( pkgselmgr->isInstalled(pp) )
	{
	    buttonimage = "uninstall.png";
	    if ( pkgselmgr->reqAction(pp)==ODInst::PkgSelMgr::Upgrade )
		statusimage =  "downarrow.png";
	    else
		statusimage = "empty.png";
	}
	else
	{
	    buttonimage = "dontinstall.png";
	    statusimage =  "downarrow.png";
	}
    }
    else
    {
	anchor.addAnchor( installcmd,  empty_string );

	if ( pkgselmgr->isInstalled(pp) )
	{
	    buttonimage = "keep.png";
	    statusimage =  "trashcan.png";
	}
	else
	{
	    buttonimage = "install.png";
	    statusimage = "empty.png";
	}
    }
    
    FilePath buttonfp( ODInst::sTempDataDir() );
    buttonfp.add( "odinst_images" );
    buttonfp.add( buttonimage );

    FilePath statusfp( ODInst::sTempDataDir() );
    statusfp.add( "odinst_images" );
    statusfp.add( statusimage );

    html.add( anchor );
    html.addImage( statusfp.fullPath().buf(), 20, 20, empty_string );
    html.addImage( buttonfp.fullPath(), 50, 20, empty_string ).add( "</a>" );
}



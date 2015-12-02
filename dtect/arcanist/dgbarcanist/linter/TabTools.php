<?php

  function tab_expand($text) {
    if ( strpos($text,"\r")!==false )
      return $text;

    $fullres = "";
    $lines = explode("\n", $text );
    foreach ($lines as $line) {
      $tab_stop = 8;
      $res = "";
      for ( $idx=0; $idx<strlen($line); $idx++ ) {
	if ( $line[$idx]=="\t" ) {
	  $curpos = strlen($res);
	  $nrtabs = (int) ($curpos/$tab_stop);
	  $newsize = ($nrtabs+1)*$tab_stop;
	  $nrspaces = $newsize-$curpos;
	  for ( $idy=$nrspaces-1; $idy>=0; $idy-- ) {
	    $res = $res." ";
	  }
	} 
	else
	  $res = $res.$line[$idx];
      }

      if ( strlen( $fullres ) )
	$fullres .= "\n".$res;
      else
	$fullres = $res;
    }

    return $fullres;
  }

  function tab_collapse( $string, $tabstop ) {
    if ( strpos($string,"\r")!==false )
      return $string;

    $fullres = "";
    $lines = explode("\n", $string );
    foreach ($lines as $line) {

      $changedline = $line;

      $length = strlen( $changedline );
      $firstchar = 0;
      $last = $tabstop-1;

      for ( $idx=$length-1; $idx>=0; $idx-- ) {

	if ( ($idx+1)%$tabstop )
	   continue;

	$nrspaces = 0;
	for ( $idy=0; $idy<$tabstop; $idy++ )
	{
	      if ( $changedline[$idx-$idy]==' ' )
		  $nrspaces++;
	      else
		  break;
	}

	if ( $nrspaces<2 )
	{
	  if ( $nrspaces==0 || ($idx<$length-1 && $changedline[$idx+1]!=="\t") )
	      continue;
	}

	$lastchartocopy = $idx-$nrspaces;
	$copysize = $lastchartocopy + 1;

	$newstring = "";
	if ( $copysize>0 )
	  $newstring .= substr( $changedline, 0, $copysize );

	$newstring .= "\t";
	$newstring .= substr( $changedline, $idx+1 );
	$changedline = $newstring;
      }

      if ( strlen( $fullres ) )
	$fullres .= "\n".$changedline;
      else
	$fullres = $changedline;
    }

    return $fullres;
  }

?>


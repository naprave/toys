#!/usr/bin/php
<?php

    $memcache = new Memcache;
    $memcache->connect('127.0.0.1', 11211) or die ("Could not connect to memcache server");

    $val = $memcache->get("camera");
    //print_r($val);
    
    $cameras = array();
//*    
    $list = array();
    $allSlabs = $memcache->getExtendedStats('slabs');
    $items = $memcache->getExtendedStats('items');
    foreach($allSlabs as $server => $slabs) {
        foreach($slabs AS $slabId => $slabMeta) {
			if( !is_numeric($slabId) ) continue;
			//print_r("\n\n".$slabId."\n");
           $cdump = $memcache->getExtendedStats('cachedump',(int)$slabId);
            foreach($cdump AS $keys => $arrVal) {
                if (!is_array($arrVal)) continue;
                foreach($arrVal AS $k => $v) {                   
					if( substr($k,0,6) == 'camera' ) {
						$cameras[substr($k,7)] = $memcache->get($k);
					}
                     //echo $k .' = '.$memcache->get($k).'<br>';
                }
           }
        }
    }   

    //print_r($cameras); //exit;
    ksort( $cameras );//sort by id
    
    //print_r($argv[1]); exit;
    $chosencam = 0;
    if($argv[1]>0) {
	$chosencam = $argv[1];
    }
    echo "chosen cam $chosencam";
    $port = 49154+$chosencam;    
    
    $ckeys = array_keys($cameras);
    $camera = $cameras[$ckeys[$chosencam]];
    print_r(" camip: $camera ");
//    foreach($cameras as $camera) 

    {
		file_get_contents("http://$camera/cam.cgi?mode=getinfo&type=capability");
		sleep(3);
		file_get_contents("http://$camera/cam.cgi?mode=getstate");
		sleep(2);
		file_get_contents("http://$camera/cam.cgi?mode=startstream&value=$port");
		

			//Create a UDP socket
		if(!($sock = socket_create(AF_INET, SOCK_DGRAM, 0)))
		{
			$errorcode = socket_last_error();
			$errormsg = socket_strerror($errorcode);
			
			die("Couldn't create socket: [$errorcode] $errormsg \n");
		}
		
		//echo "Socket created \n";
		
		// Bind the source address
		if( !socket_bind($sock, "0.0.0.0" , $port) )
		{
			$errorcode = socket_last_error();
			$errormsg = socket_strerror($errorcode);
			
			die("Could not bind socket : [$errorcode] $errormsg \n");
		}
		
		//echo "Socket bind OK \n";
		$port++;
    }
	
	$lasttime = time();
	//Do some communication, this loop can handle multiple clients
	$packt = 1;
	while(1)
	{
		//echo "Waiting for data ... \n";
		
		//Receive some data
		$r = socket_recvfrom($sock, $buf, 4096*12, 0, $remote_ip, $remote_port);
// 		echo "$remote_ip : $remote_port -- " . $buf;
		//echo $buf;
		
		//file_put_contents("packet$packt.udp", $buf);
		if( $r> 1000 ) {
		    $data = substr($buf,44);
/*		    
		    echo substr($buf,44);
/*/
		    file_put_contents($ckeys[$chosencam], $data, FILE_APPEND);
/**/
		}
		//Send back the data to the client
		//socket_sendto($sock, "OK " . $buf , 100 , 0 , $remote_ip , $remote_port);
		
		if( ($lasttime+10) < time() ) {
			//echo 'getstating';
			$lasttime = time();
			file_get_contents("http://$camera/cam.cgi?mode=getstate");
		}
		$packt++;
	}
 
    
/**/    
?>
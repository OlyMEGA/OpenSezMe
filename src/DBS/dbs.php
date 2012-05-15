<?php

include "../../lib/php_serial.class.php";

class dbs
{
	// Let's start the class
	$serial = new phpSerial;

	function dbs() {
		$serial->deviceSet("/dev/cu.usbmodem12341");
		$serial->confBaudRate(9600);
		$serial->confParity("none");
		$serial->confCharacterLength(8);
		$serial->confStopBits(1);
		$serial->confFlowControl("none");
		$serial->deviceOpen();		
	}

	//$serial->sendMessage("H");

	// Or to read from
	while (true) {
	    $read = $serial->readPort();
	    if ($read != null) {
	        print($read);
	        $serial->sendMessage("H");
	    }
	}

	// If you want to change the configuration, the device must be closed
	$serial->deviceClose();
}

?>

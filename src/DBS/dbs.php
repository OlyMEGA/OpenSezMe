<?php

include "../../lib/php/php_serial.class.php";

class dbs
{
	// Let's start the class
    public $serial;

    function dbs() {
        $this->serial = new phpSerial;

        $mySerial = $this->serial;
        $mySerial->deviceSet("/dev/cu.usbmodem12341");
		$mySerial->confBaudRate(9600);
		$mySerial->confParity("none");
		$mySerial->confCharacterLength(8);
		$mySerial->confStopBits(1);
		$mySerial->confFlowControl("none");
		$mySerial->deviceOpen();
	}

    function go()
    {
        $mySerial = $this->serial;

        //$serial->sendMessage("H");

        // Or to read from
        while (true) {
            $read = $mySerial->readPort();
            if ($read != null) {
                print($read);
                $mySerial->sendMessage("H");
            }
        }

        // If you want to change the configuration, the device must be closed
        $mySerial->deviceClose();

    }
}

?>

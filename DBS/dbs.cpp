
#include <QVariant>
#include <QDebug>
#include <QTimer>

#include <QDateTime>

#include <iostream>
using namespace std;

#include "dbs.h"

DBS::DBS(QObject* parent)
        : QObject(parent)
{
    port = new QextSerialPort("/dev/ttyUSB0");
    //port = new QextSerialPort("/dev/ttyUSB1");
    //port = new QextSerialPort("/dev/ttyS0");
    //port->setBaudRate(BAUD9600);
    port->setBaudRate(BAUD57600);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_1);
    //set timeouts to 500 ms
    port->setTimeout(500);

    port->open(QIODevice::ReadWrite | QIODevice::Unbuffered);

    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("omega");
    db.setUserName("omega");
    db.setPassword("U7cBmr6K5LttPeQP"); // TODO: hide this! WTF?!
    bool ok = db.open();

    _cmdInProcess = 0;
    _txPingSent = false;
    _rcvCheckSum = 0;
    _rcvState = RCV_IDLE;
    _dataMode = DATA_MODE_IDLE;

    // Move this object to a newly created thread.
    dbsThread = new DBSThread();
    moveToThread(dbsThread);

    // Connect the thread started signal to this objects' initialization
    connect(dbsThread, SIGNAL(started()), this, SLOT(initSystem()));
    dbsThread->start();
}



////////////////////////////////////////////////////////////////////////////
//  initSystem --
// Initialize thread timers and signal/slot messaging
void DBS::initSystem() {
    receiveTimer = new QTimer(this);
    connect(receiveTimer, SIGNAL(timeout()), this, SLOT(receiveData()));
    receiveTimer->start(1);

    pingTimer = new QTimer(this);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(pingThing()));
    pingTimer->start(500);

    // Connect command sending signals to appropriate slot
    connect(this, SIGNAL(handleCmd()), this, SLOT(processCommand()));
}


////////////////////////////////////////////////////////////////////////////
//  receiveData --
// This is called every 10 ms using a QTimer signal
// This function should "consume" the data from the receive
// buffer and send signals when valid packets are found
void DBS::receiveData() {
    // Read bytes from the input buffer one at a time and process
    // them through the packet read state machine
    // When a complete, valid packet is read, send a signal handleCmd()

    unsigned char rcvByte;
    QString debugMsg;
    int i = 0;
    int j = 0;

    if( port->isOpen() ) {
        if( port->bytesAvailable() && !_cmdInProcess ) {
            if( (i = port->read((char*)&rcvByte, 1)) == 1 ) {
                j = processPacket(rcvByte);
            }
        }
    }
}



void DBS::pingThing() {
    unsigned char mode = DATA_MODE_IDLE;

//    // Data modes
//#define DATA_MODE_IDLE 0x00
//#define DATA_MODE_CODES 0x01
//#define DATA_MODE_ACTIONS 0x02
//#define DATA_MODE_LCD_MSGS 0x03
//#define DATA_MODE_AUDIT 0x04

    //sendACAUPing(CMD_PING_REQ);

//    mode = DATA_MODE_LCD_MSGS;
//    writePacket(CMD_CHG_MODE, 1, (char*)&mode, 0);
//
//    //                 12345678901234567890123456789012
//    char lcdText1[] = "This is test 111";
//    writePacket(CMD_DATA_PKT, strlen(lcdText1), lcdText1, 0);
//
//    //                 12345678901234567890123456789012
//    char lcdText2[] = "This is test 222";
//    writePacket(CMD_DATA_PKT, strlen(lcdText2), lcdText2, 1);
//
//    //                 12345678901234567890123456789012
//    char lcdText3[] = "This is test 333";
//    writePacket(CMD_DATA_PKT, strlen(lcdText3), lcdText3, 2);

    mode = DATA_MODE_IDLE;
    //writePacket(CMD_CHG_MODE, 1, (char*)&mode, 0);

    mode = DATA_MODE_CODES;
    //writePacket(CMD_CHG_MODE, 1, (char*)&mode, 0);

    char codeText1[] = "0123456789";
    //sendCode(1, codeText1, 0);

    char codeText2[] = "9876543210";
    //sendCode(1, codeText2, 1);

    char codeText3[] = "9999";
    //sendCode(2, codeText3, 2);

    char codeText4[] = "1234";
    //sendCode(99, codeText4, 3);

    mode = DATA_MODE_ACTIONS;
    //writePacket(CMD_CHG_MODE, 1, (char*)&mode, 0);

    mode = DATA_MODE_AUDIT;
    //writePacket(CMD_CHG_MODE, 1, (char*)&mode, 0);
}



////////////////////////////////////////////////////////////////////////////
//  processPacket --
int DBS::processPacket(unsigned char rcvByte) {
    // Tally up the checksum from all packet bytes received
    _rcvCheckSum += rcvByte;

    QString debugMsg22;

#if 0
    QString debugMsg = QString("%1:")
                    //.arg((int)rcvByte, 2, 16, QChar('0')).toUpper()
                    .arg((int)rcvByte, 2, 16, QChar('0')).toUpper();

    if( rcvByte == 0xA5 && _rcvState == RCV_IDLE ) {
            cout << endl << flush;
    }

    cout << debugMsg.toStdString() << flush;
#endif

    switch(_rcvState) {
            case RCV_IDLE:
                    // read preamble byte, get a "lock" on the packet
                    if( rcvByte == 0xA5 ) {
                            _rcvState = RCV_BEGIN;
                    }

                    break;

            case RCV_BEGIN:
                    _rcvType = rcvByte;

                    if( _rcvType < CMD_LAST ) {
                            _rcvState = RCV_TYPE;
                    } else {
                            // Bail! this is not a valid packet.
                            _rcvState = RCV_IDLE;
                            _rcvCheckSum = 0;
                    }

                    break;

            case RCV_TYPE:
                    // We now have the packet type now, read the size
                    _rcvSize = rcvByte;

                    if( _rcvSize < 33 ) {
                        // Now we have the size of the data
                        // if it is zero then it changes the way we read the rest of the packet
                        if( _rcvSize > 0 ) {
                            _rcvState = RCV_RECNUMH;
                        } else {
                            _rcvState = RCV_CHKSUM;
                        }
                    } else {
                            // This looks to be a bad packet
                            _rcvState = RCV_IDLE;
                            _rcvCheckSum = 0;
                    }

                    break;

            case RCV_RECNUMH:
                    _rcvRecNum = rcvByte << 8;
                    _rcvState = RCV_RECNUML;
                    break;

            case RCV_RECNUML:
                    _rcvRecNum |= rcvByte;
                    _rcvState = RCV_DATA;
                    _rcvCount = 0; // This is important!
                    break;

            case RCV_DATA:
                    // Now we should be receiving data bytes
                    // Count them off using rcvSize
                    _rcvData[_rcvCount++] = rcvByte;

                    if( _rcvCount >= _rcvSize ) {
                        _rcvState = RCV_CHKSUM;
                    }

                    break;

            case RCV_CHKSUM:
                    // This must be the checksum byte
                    // If all goes well here, we can process the packet's contents

                    if( _rcvCheckSum == 0x00 ) {
                            // All good, go ahead and use this data/command
                            // TODO: Do we send an ACK?

                            _rcvState = RCV_IDLE;
                            _rcvCheckSum = 0;

                            _cmdInProcess = 1;
                            emit handleCmd();

                            // Return a value indicating there is a valid command to process
                            return( 1 );
                    } else {
                            // This is the end-of-the-line
                            // Bad packet
                            // TODO: Do we send a NACK
                            _rcvState = RCV_IDLE;
                            _rcvCheckSum = 0;

                            cout << "BAD PACKET!" << endl << flush;
                    }

                    break;

            default:
                    // Don't know what happened here buuuut, lets get right
                    _rcvState = RCV_IDLE;
                    _rcvCheckSum = 0;
                    cout << "Something strange happened!" << endl << flush;
                    break;
    }

    return( 0 );
}




////////////////////////////////////////////////////////////////////////////
//  processCommand -- This is a slot
void DBS::processCommand() {
    int thisCommand = 0;
    char debugMsg[256];

    switch( _rcvType ) {
        case CMD_PING_REQ:
            // return a ping reply here
            sendACAUPing(CMD_PING_RESP);

            break;

        case CMD_PING_RESP:
            // Check if this is a valid response (did we send a ping request?)
            if( _txPingSent ) {
                //
                _txPingSent = false;

                cout << "Got PING response!" << endl << flush;
            }

            break;

        case CMD_GEN_CMD:
            thisCommand = _rcvData[0];

            if( thisCommand == 0 ) {
                break;
            }

            // CMD_GEN_DEBUG
            if( thisCommand == CMD_GEN_DEBUG ) {
                // Rest of _rcvData is debug text

                strncpy(debugMsg, (const char*)&_rcvData[1], _rcvSize - 1);
                debugMsg[_rcvSize - 1] = 0;

                // Just print it to STDOUT
                cout << "" << debugMsg << "" << flush;

                break;
            }

            break;

        // The DBS does not need this command because the ACAU cannot change the data mode
        case CMD_CHG_MODE:

            break;

        case CMD_DATA_PKT:
            // Copy the data record in rcvData[] to the correct area (internal EEPROM, external, etc)
            // depending on the current data mode.
            // If data received in Idle Mode then there is an error
            switch( _dataMode ) {
                case DATA_MODE_IDLE:
                    // Error! We got qa data packet in idle mode
                    // Do nothing with the data
                    break;

                case DATA_MODE_AUDIT:
                    // Compare and write new data into DB
                    break;
            }
            break;

        default:
            // Handle unknown data mode specifications here. Break in error?
            break;
    }

    // Indicate there is no longer a command being processed
    _cmdInProcess = 0;
}



// DBS --> ACAU: A5 00 00 5B = Ping request
// ACAU --> DBS: A5 01 00 5A = Ping response
int DBS::sendACAUPing(unsigned char pingType) {
        unsigned char packetToSend[4];

        if( pingType == CMD_PING_REQ ) {
                packetToSend[0] = 0xA5;
                packetToSend[1] = 0x00;
                packetToSend[2] = 0x00;
                packetToSend[3] = 0x5B;

                //cout << "Gonna send ping requset" << endl << flush;

                port->write((char*)packetToSend, 4);
                _txPingSent = true;

                QTimer::singleShot(100, this, SLOT(pingTimeout()));
        } else {
                packetToSend[0] = 0xA5;
                packetToSend[1] = 0x01;
                packetToSend[2] = 0x00;
                packetToSend[3] = 0x5A;

                //cout << "sending response..." << endl << flush;

                port->write((char*)packetToSend, 4);
        }
}



// pingTimeout -- This is a slot
void DBS::pingTimeout() {
    //
    _txPingSent = false;
}



void DBS::writePacket(unsigned char packetType, unsigned char size, char* data, int recnum) {
        unsigned char packetToSend[64];
        unsigned char checksum = (unsigned char)0xA5 + packetType + size;

        packetToSend[0] = 0xA5; // First things first
        packetToSend[1] = packetType;
        packetToSend[2] = size;

        if( size > 0 ) {
                packetToSend[3] = (unsigned char)((recnum & 0xFF00) >> 8);
                packetToSend[4] = (unsigned char)(recnum & 0x00FF);
                checksum += (packetToSend[3] + packetToSend[4]);

                for( int i = 0; i < size; i++ ) {
                        packetToSend[5 + i] = data[i];
                        checksum += data[i];
                }
        }

        packetToSend[5 + size] = (unsigned char)(-checksum);

        port->write((const char*)packetToSend, 6 + size);
}




void DBS::sendCode(unsigned char action, char *code, int recnum) {
    // Start with action code byte
    QByteArray codeToSend((const char*)&action, 1);

    // Append the actual code
    codeToSend.append(code);

    // Pad it out with zeros to fill 11 bytes total
    codeToSend.append(QByteArray::QByteArray(10 - strlen(code), '\0'));

    // TODO: Add a check here that the proper data mode has been selected
    // Write this command packet to the ACAU for storage in EEPROM DB
    writePacket(CMD_DATA_PKT, 11, codeToSend.data(), recnum);
}


//void DBS::sendAction(unsigned char action, char *code, int recnum) {
//    // Start with action code byte
//    QByteArray codeToSend((const char*)&action, 1);
//
//    // Append the actual code
//    codeToSend.append(code);
//
//    // Pad it out with zeros to fill 11 bytes total
//    codeToSend.append(QByteArray::QByteArray(10 - strlen(code), '\0'));
//
//    // TODO: Add a check here that the proper data mode has been selected
//    // Write this command packet to the ACAU for storage in EEPROM DB
//    writePacket(CMD_DATA_PKT, 11, codeToSend.data(), recnum);
//}




///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


// OLD DATABASE STUFF -- save for later


//void DBS::receiveMsg()
//{
//    char buff[1024];
//    int numBytes;
//    int memberid = 0;
//    int numResults = 0;
//    int activityid = 0;
//    QByteArray wholeBuffer;
//    QString CardNum;
//    QString PinCode;
//    RESULT_REGISTERS userData;
//
//    //wholeLine = QByteArray::fromHex(fields.at(0).toLocal8Bit().constData());
//
//    if( port->isOpen() ) {
//        //port->write("S650FP"); // Read 15 bytes from the slave
//
//        numBytes = port->bytesAvailable();
//        if (numBytes != -1) {
//            //cout << numBytes << endl << flush;
//            if(numBytes > 1024)
//                numBytes = 1024;
//
//            int i = port->read(buff, numBytes);
//            if (i != -1)
//                        buff[i] = '\0';
//                else
//                        buff[0] = '\0';
//           this->received_text = buff;
//           //received_text.truncate(10);
//
//           //cout << received_text.toStdString() << endl << flush;
//
//           wholeBuffer = QByteArray::fromHex(received_text.toLocal8Bit().constData());
//
//           switch( wholeBuffer[0] & 0x03 ) {
//               case RESULT_REG_EVENT_CARDSWIPED:
//                   //port->write("S64001FP"); // test
//
//                   for(int j = 0; j < 10; j++ ) {
//                       CardNum.append(wholeBuffer.at(j+1));
//                   }
//
//                   //checkDB( CardNum );
//
//                   break;
//
//               case RESULT_REG_EVENT_PINENT:
//                   //port->write("S64002FP"); // test
//
//                   for(int j = 0; j < 4; j++ ) {
//                       PinCode.append(wholeBuffer.at(j+11));
//                   }
//
//                   cout << PinCode.toStdString() << endl << flush;
//
//                   break;
//
//               case RESULT_REG_EVENT_TAMPER:
//                   //port->write("S64004FP"); // test
//                   break;
//           }
//        } // End if( numBytes != -1 )
//    }
//}




//void DBS::checkDB(QString received_text) {
//    int numResults = 0;
//    int memberid = 0;
//
//#if 1
//           QSqlQuery query(db);
//           query.prepare("SELECT memberid, mbrfirstname, mbrlastname, mbrcardno FROM member WHERE mbrcardno = :cardno LIMIT 1");
//           query.bindValue(":cardno", QVariant(received_text));
//           query.exec();
//
//           numResults = query.size();
//
//           if( numResults > 0 ) {
//               query.next();
//               memberid = query.value(0).toInt();
//               QString first = query.value(1).toString();
//               QString last = query.value(2).toString();
//
//               cout << first.toStdString() << " " << last.toStdString() << endl << flush;
//
//               QSqlQuery status_query(db);
//               status_query.prepare("SELECT activityid, memberid, actdate, acttimein, acttimeout, TIMESTAMPDIFF(HOUR,CONCAT(actdate, ' ', acttimein) ,NOW()) FROM activity WHERE memberid = :memberid AND acttimeout = '00:00:00' ORDER BY actdate DESC, acttimein DESC LIMIT 1");
//               status_query.bindValue(":memberid", QVariant(memberid));
//               status_query.exec();
//
//               if( status_query.size() > 0 ) {
//                   status_query.next();
//                   int activityid = status_query.value(0).toInt();
//                   QString date = status_query.value(2).toString();
//                   QString intime = status_query.value(3).toString();
//                   QString outtime = status_query.value(4).toString();
//                   int hoursago = status_query.value(5).toInt();
//
////                   cout     << "Last Login: "
////                            << date.toStdString() << " "
////                            << intime.toStdString() << " "
////                            << outtime.toStdString() << "  Over "
////                            << hoursago << " hours ago" << endl << flush;
//
//                   if( hoursago < 12 ) {
//                       QSqlQuery punchin_query;
//                       punchin_query.prepare("UPDATE activity SET acttimeout = :acttimeout WHERE activityid = :activityid");
//                       punchin_query.bindValue(":activityid", QVariant(activityid));
//                       QDateTime dateNow = QDateTime::currentDateTime();
//                       punchin_query.bindValue(":acttimeout", dateNow);
//                       punchin_query.exec();
//
//                       cout << "==== OUT OUT OUT ====" << endl << flush;
//
//                       // Send sound command
//                       //port->write("B"); // bee Doop
//                       port->write("S64002FP"); // bee Doop
//                       sendLCDText("Logging OUT");
//
//                   } else {
//                       QSqlQuery punchin_query;
//                       punchin_query.prepare("INSERT INTO activity (memberid, actdate, acttimein) "
//                                                "VALUES (:memberid, :actdate, :acttimein)");
//                       punchin_query.bindValue(":memberid", QVariant(memberid));
//                       QDateTime dateNow = QDateTime::currentDateTime();
//                       punchin_query.bindValue(":actdate", dateNow);
//                       punchin_query.bindValue(":acttimein", dateNow);
//                       punchin_query.exec();
//
//                       cout << "==== IN IN IN ====" << endl << flush;
//
//                       // Send sound command
//                       //port->write("A"); // Boo Deep
//                       port->write("S64001FP"); // Boo Deep
//                       sendLCDText("Logging in");
//                   }
//               } else {
//                   //if( memberid > 0 ) {
//                       QSqlQuery punchin_query;
//                       punchin_query.prepare("INSERT INTO activity (memberid, actdate, acttimein) "
//                                                "VALUES (:memberid, :actdate, :acttimein)");
//                       punchin_query.bindValue(":memberid", QVariant(memberid));
//                       QDateTime dateNow = QDateTime::currentDateTime();
//                       punchin_query.bindValue(":actdate", dateNow);
//                       punchin_query.bindValue(":acttimein", dateNow);
//                       punchin_query.exec();
//
//                       cout << "==== IN IN IN ====" << endl << flush;
//
//                       // Send sound command
//                       //port->write("A"); // Boo Deep
//                       port->write("S64001FP"); // Boo Deep
//                       sendLCDText("Logging in");
//                   //}
//               }
//
//           } else {
//
//               // Send sound command
//               //port->write("C"); // booop
//               port->write("S64003FP"); // booop
//               sendLCDText("ERROR");
//               cout << "[Card not found]" << endl << flush;
//           }
//#endif
//       }
//
//void DBS::sendLCDText(QString textIn ) {
//    QByteArray textBuffer;
//    QByteArray outputText("S64000402");
//
//    textBuffer.append(textIn.toAscii());
//    outputText.append(textBuffer.toHex());
//
//    outputText.append("00P");
//
//    //port->write(outputText); // test
//}

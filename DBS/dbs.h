#ifndef DBS_H
#define DBS_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QSqlError>
#include <qextserialport.h>

#define RESULT_REG_EVENT_STATUS 0
#define RESULT_REG_EVENT_CARDSWIPED 1
#define RESULT_REG_EVENT_PINENT 2
#define RESULT_REG_EVENT_TAMPER 3

#define RFID_CODE_LEN 10
#define PIN_CODE_LEN 4


// Raw, low-level packet states
#define RCV_IDLE 0x00
#define RCV_BEGIN 0x01
#define RCV_TYPE 0x02
#define RCV_RECNUMH 0x03
#define RCV_RECNUML 0x04
#define RCV_DATA 0x05
#define RCV_CHKSUM 0x06

// Packet type codes
#define CMD_PING_REQ 0x00
#define CMD_PING_RESP 0x01
#define CMD_GEN_CMD 0x11
#define CMD_GEN_DEBUG 0x99
#define CMD_CHG_MODE 0x12

// Data modes
#define DATA_MODE_IDLE 0x00
#define DATA_MODE_CODES 0x01
#define DATA_MODE_ACTIONS 0x02
#define DATA_MODE_LCD_MSGS 0x03
#define DATA_MODE_AUDIT 0x04

#define CMD_DATA_PKT 0x20
#define CMD_NOP 0x30 // Placeholder for more commands
#define CMD_LAST 0x40 // Used for rudimentary validity check during receive

//
#define OPEN_DOOR_ALWAYS 0x01
#define OPEN_DOOR_DATE_RANGE 0x02
#define OPEN_DOOR_LIMITED_USE 0x03
#define OPEN_DOOR_WEEKDAY_MASK 0x04
#define OPEN_DOOR_DBS_REQUEST 0x05
#define POWER_ON_EQUIPMENT 0x06
#define OTHER_ACTION 0x07
#define SOUND_ALARM 0x09


struct RESULT_REGISTERS{
  unsigned char Status;
  char CardNum[RFID_CODE_LEN];
  char PINCode[PIN_CODE_LEN];
  //char SerialNum[11];
};


class DBSThread : public QThread{
    public:
        DBSThread(QObject *parent = 0) : QThread(parent) {

        }

        static void msleep(int ms) {
                QThread::msleep(ms);
        }

    protected:
        void run() {
                exec();
        }
};


class DBS : public QObject
{
    Q_OBJECT

    public:
        DBS(QObject *parent=0);

        void writePacket(unsigned char packetType, unsigned char size, char* data, int recnum);
        void sendCode(unsigned char action, char *code, int recnum);

        private:
        QextSerialPort *port;

        // A thread to hold our object so we can read data async
        DBSThread *dbsThread;

        //void checkDB(QString received_text);
        void sendLCDText(QString textIn);


        int processPacket(unsigned char rcvByte);

    private:
        QString received_text;
        QSqlDatabase db;

        QTimer *receiveTimer;
        QTimer *pingTimer;

        int _cmdInProcess;

        unsigned char _rcvState;
        unsigned char _rcvCount;

        unsigned char _dataMode;

        unsigned char _rcvCheckSum;
        int _rcvRecNum;
        unsigned char _rcvSize;
        unsigned char _rcvType;
        unsigned char _rcvData[32];

        bool _txPingSent;
        int sendACAUPing(unsigned char pingType);

    private slots:

        void initSystem();
        //void transmitMsg();
        //void transmitCommand();
        void receiveData();

        void pingTimeout();
        void pingThing();

        void processCommand();
        //void receiveMsg();


    signals:
        void handleCmd();
//        void cmdSent();
//        void dataReady();
};

#endif // DBS_H

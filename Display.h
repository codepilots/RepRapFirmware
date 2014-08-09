/****************************************************************************************************

RepRapFirmware - Display

This class represents a Display, it is heavily based on the web sever.



-----------------------------------------------------------------------------------------------------

Version 0.

19 July 2014

Chris Thomson
Codepilots.com

Licence: GPL

****************************************************************************************************/

#ifndef Display_H
#define Display_H

#include "interface.h"
#include "Displays/IDisplayType.h"

#define KO_START "rr_"
#define KO_FIRST 3

//TODO: Make sure these are the constants used in the code if they need to be distinct from the web server, or delete.
const unsigned int POgcodeBufLength = 2048;		// size of our gcode ring buffer, ideally a power of 2
const unsigned int POminReportedFreeBuf = 100;	// the minimum free buffer we report if not zero
const unsigned int POmaxReportedFreeBuf = 900;	// the max we own up to having free, to avoid overlong messages

class Display
{   
  public:
  
    Display(Platform* p);
    bool GCodeAvailable();
    byte ReadGCode();
    bool WebserverIsWriting();
    void Init();
    void Spin();
    void Exit();
    void Diagnostics();
    void SetPassword(const char* pw);
    void SetName(const char* nm);
    void ConnectionError();
    void WebDebug(bool wdb);

    friend class Platform;

  protected:

    void MessageStringToWebInterface(const char *s, bool error);
    void AppendReplyToWebInterface(const char* s, bool error);

  private:
  
    void ParseClientLine();
    void SendFile(const char* nameOfFileToSend);
    bool WriteBytes();
    void ParseQualifier();
    void CheckPassword();
    void LoadGcodeBuffer(const char* gc, bool convertWeb);
    void CloseClient();
    bool PrintHeadString();
    bool PrintLinkTable();
    void GetGCodeList();
    void GetJsonResponse(const char* request);
    void ParseGetPost();
    bool CharFromClient(char c);
    void BlankLineFromClient();
    void InitialisePost();
    bool MatchBoundary(char c);
    void JsonReport(bool ok, const char* request);
    unsigned int GetGcodeBufferSpace() const;
    unsigned int GetReportedGcodeBufferSpace() const;
    void ProcessGcode(const char* gc);

    Platform* platform;
    bool active;
    float lastTime;
    float longWait;
    FileStore* fileBeingSent;
    bool writing;
    bool receivingPost;
    int boundaryCount;  
    FileStore* postFile;
    bool postSeen;
    bool getSeen;
    bool clientLineIsBlank;
    float clientCloseTime;
    bool needToCloseClient;

    char gcodeBuffer[POgcodeBufLength];
    unsigned int gcodeReadIndex, gcodeWriteIndex;		// head and tail indices into gcodeBuffer
    int jsonPointer;
    int clientLinePointer;
    bool gotPassword;
    uint16_t seq;	// reply sequence number, so that the client can tell if a reply is new or not
    bool webDebug;
};

#endif

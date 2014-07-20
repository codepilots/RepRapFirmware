/****************************************************************************************************

RepRapFirmware - PanelOne

This class represents a PanelOne, it is heavily based on the web sever.



-----------------------------------------------------------------------------------------------------

Version 0.

19 July 2014

Chris Thomson
Codepilots.com

Licence: GPL

****************************************************************************************************/

#include "RepRapFirmware.h"

//****************************************************************************************************

// Feeding G Codes to the GCodes class

bool Panelone::GCodeAvailable()
{
  return gcodeReadIndex != gcodeWriteIndex;
}

byte Panelone::ReadGCode()
{
  byte c;
  if (gcodeReadIndex == gcodeWriteIndex)
  {
	  c = 0;
  }
  else
  {
	  c = gcodeBuffer[gcodeReadIndex];
	  gcodeReadIndex = (gcodeReadIndex + 1u) % gcodeBufLength;
  }
  return c;
}

// Process a received string of gcodes
void Panelone::LoadGcodeBuffer(const char* gc, bool convertWeb)
{
	char gcodeTempBuf[GCODE_LENGTH];
	uint16_t gtp = 0;
	bool inComment = false;
	for (;;)
	{
		char c = *gc++;
		if(c == 0)
		{
			gcodeTempBuf[gtp] = 0;
			ProcessGcode(gcodeTempBuf);
			return;
		}

		if(c == '+' && convertWeb)
	    {
			c = ' ';
	    }
	    else if(c == '%' && convertWeb)
	    {
	    	c = *gc++;
	    	if(c != 0)
	    	{
	    		unsigned char uc;
	    		if(isalpha(c))
	    		{
	    			uc = 16*(c - 'A' + 10);
	    		}
	    		else
	    		{
	    			uc = 16*(c - '0');
	    		}
	    		c = *gc++;
	    		if(c != 0)
	    		{
	    			if(isalpha(c))
	    			{
	    				uc += c - 'A' + 10;
	    			}
	    			else
	    			{
	    				uc += c - '0';
	    			}
	    			c = uc;
	    		}
	    	}
	    }

		if (c == '\n')
		{
			gcodeTempBuf[gtp] = 0;
			ProcessGcode(gcodeTempBuf);
			gtp = 0;
			inComment = false;
		}
		else
		{
			if (c == ';')
			{
				inComment = true;
			}

			if(gtp == ARRAY_SIZE(gcodeTempBuf) - 1)
			{
				// gcode is too long, we haven't room for another character and a null
				if (c != ' ' && !inComment)
				{
					platform->Message(BOTH_ERROR_MESSAGE, "Panelone: GCode local buffer overflow.\n");
					return;
				}
				// else we're either in a comment or the current character is a space.
				// If we're in a comment, we'll silently truncate it.
				// If the current character is a space, we'll wait until we see a non-comment character before reporting an error,
				// in case the next character is end-of-line or the start of a comment.
			}
			else
			{
				gcodeTempBuf[gtp++] = c;
			}
		}
	}
}

// Process a null-terminated gcode
// We intercept four G/M Codes so we can deal with file manipulation and emergencies.  That
// way things don't get out of sync, and - as a file name can contain
// a valid G code (!) - confusion is avoided.

void Panelone::ProcessGcode(const char* gc)
{
  int8_t specialAction = 0;
  if(StringStartsWith(gc, "M30 "))
  {
	  specialAction = 1;
  }
  else if(StringStartsWith(gc, "M23 "))
  {
	  specialAction = 2;
  }
  else if(StringStartsWith(gc, "M112") && !isdigit(gc[4]))
  {
	  specialAction = 3;
  }
  else if(StringStartsWith(gc, "M503") && !isdigit(gc[4]))
  {
	  specialAction = 4;
  }
  
  if(specialAction != 0) // Delete or print a file?
  { 
    switch (specialAction)
    {
    case 1: // Delete
      reprap.GetGCodes()->DeleteFile(&gc[4]);
      break;

    case 2:	// print
      reprap.GetGCodes()->QueueFileToPrint(&gc[4]);
      break;

    case 3:
      reprap.EmergencyStop();
      break;

    case 4:
	  {
		FileStore *configFile = platform->GetFileStore(platform->GetSysDir(), platform->GetConfigFile(), false);
		if(configFile == NULL)
		{
		  platform->Message(WEB_ERROR_MESSAGE, "Configuration file not found");
		}
		else
		{
		  char c;
		  size_t i = 0;
		  /*
		  while(i < STRING_LENGTH && configFile->Read(c))
		  {
			gcodeReply[i++] = c;
		  }
		  configFile->Close();
		  gcodeReply[i] = 0;
		  */
		  ++seq;
		}
	  }
	  break;
    }
  }
  else
  {
	  // Copy the gcode to the buffer
	  size_t len = strlen(gc) + 1;		// number of characters to copy
	  if (len > GetGcodeBufferSpace())
		  platform->Message(BOTH_ERROR_MESSAGE, "Panelone: GCode buffer overflow.\n");
	  else
	  {
		  size_t remaining = gcodeBufLength - gcodeWriteIndex;
		  if (len <= remaining)
		  {
			  memcpy(&gcodeBuffer[gcodeWriteIndex], gc, len);
		  }
		  else
		  {
			  memcpy(&gcodeBuffer[gcodeWriteIndex], gc, remaining);
			  memcpy(gcodeBuffer, gc + remaining, len - remaining);
		  }
		  gcodeWriteIndex = (gcodeWriteIndex + len) % gcodeBufLength;
	  }
  }
}

//********************************************************************************************

// Communications with the client

//--------------------------------------------------------------------------------------------

// Output to the client

void Panelone::CloseClient()
{
  writing = false;
  //inPHPFile = false;
  //InitialisePHP();
  clientCloseTime = platform->Time();
  needToCloseClient = true;   
}


void Panelone::GetJsonResponse(const char* request)
{
  jsonPointer = 0;
  writing = true;
  //TODO: Should be able to rework this to report key values to the panel dashboard
  /*
  if(StringStartsWith(request, "poll"))
  {
    strncpy(jsonResponse, "{\"poll\":[", STRING_LENGTH);
    if(reprap.GetGCodes()->PrintingAFile())
    	strncat(jsonResponse, "\"P\",", STRING_LENGTH); // Printing
    else
    	strncat(jsonResponse, "\"I\",", STRING_LENGTH); // Idle

    float liveCoordinates[DRIVES+1];
    reprap.GetMove()->LiveCoordinates(liveCoordinates);
    for(int8_t drive = 0; drive < AXES; drive++)
    {
    	strncat(jsonResponse, "\"", STRING_LENGTH);
    	snprintf(scratchString, STRING_LENGTH, "%.2f", liveCoordinates[drive]);
    	strncat(jsonResponse, scratchString, STRING_LENGTH);
    	strncat(jsonResponse, "\",", STRING_LENGTH);
    }

    // FIXME: should loop through all Es

    strncat(jsonResponse, "\"", STRING_LENGTH);
    snprintf(scratchString, STRING_LENGTH, "%.4f", liveCoordinates[AXES]);
    strncat(jsonResponse, scratchString, STRING_LENGTH);
    strncat(jsonResponse, "\",", STRING_LENGTH);

    for(int8_t heater = 0; heater < HEATERS; heater++)
    {
      strncat(jsonResponse, "\"", STRING_LENGTH);
      snprintf(scratchString, STRING_LENGTH, "%.1f", reprap.GetHeat()->GetTemperature(heater));
      strncat(jsonResponse, scratchString, STRING_LENGTH);
      if(heater < HEATERS - 1)
    	  strncat(jsonResponse, "\",", STRING_LENGTH);
      else
    	  strncat(jsonResponse, "\"", STRING_LENGTH);
    }

    strncat(jsonResponse, "]", STRING_LENGTH);

    // Send the Z probe value

    if (platform->GetZProbeType() == 2)
    {
    	snprintf(scratchString, STRING_LENGTH, ",\"probe\":\"%d (%d)\"", (int)platform->ZProbe(), platform->ZProbeOnVal());
    }
    else
    {
    	snprintf(scratchString, STRING_LENGTH, ",\"probe\":\"%d\"", (int)platform->ZProbe());
    }
    strncat(jsonResponse, scratchString, STRING_LENGTH);

    // Send the amount of buffer space available for gcodes
   	snprintf(scratchString, STRING_LENGTH, ",\"buff\":%u", GetReportedGcodeBufferSpace());
   	strncat(jsonResponse, scratchString, STRING_LENGTH);

    // Send the home state. To keep the messages short, we send 1 for homed and 0 for not homed, instead of true and false.
    strncat(jsonResponse, ",\"hx\":", STRING_LENGTH);
    strncat(jsonResponse, (reprap.GetGCodes()->GetAxisIsHomed(0)) ? "1" : "0", STRING_LENGTH);
    strncat(jsonResponse, ",\"hy\":", STRING_LENGTH);
    strncat(jsonResponse, (reprap.GetGCodes()->GetAxisIsHomed(1)) ? "1" : "0", STRING_LENGTH);
    strncat(jsonResponse, ",\"hz\":", STRING_LENGTH);
    strncat(jsonResponse, (reprap.GetGCodes()->GetAxisIsHomed(2)) ? "1" : "0", STRING_LENGTH);

    // Send the response sequence number
    strncat(jsonResponse, ",\"seq\":", STRING_LENGTH);
	snprintf(scratchString, STRING_LENGTH, "%u", (unsigned int)seq);
	strncat(jsonResponse, scratchString, STRING_LENGTH);

    // Send the response to the last command. Do this last because it is long and may need to be truncated.
    strncat(jsonResponse, ",\"resp\":\"", STRING_LENGTH);
    size_t jp = strnlen(jsonResponse, STRING_LENGTH);
    const char *p = gcodeReply;
    while (*p != 0 && jp < STRING_LENGTH - 2)	// leave room for the final '"}'
    {
    	char c = *p++;
    	char esc;
    	switch(c)
    	{
    	case '\r':
    		esc = 'r'; break;
    	case '\n':
    		esc = 'n'; break;
    	case '\t':
    		esc = 't'; break;
    	case '"':
    		esc = '"'; break;
    	case '\\':
    		esc = '\\'; break;
    	default:
    		esc = 0; break;
    	}
    	if (esc)
    	{
    		if (jp >= STRING_LENGTH - 3)
    			break;
   			jsonResponse[jp++] = '\\';
   			jsonResponse[jp++] = esc;
    	}
    	else
    	{
    		jsonResponse[jp++] = c;
    	}
    }
    strncat(jsonResponse, "\"}", STRING_LENGTH);

    jsonResponse[STRING_LENGTH] = 0;
    gcodeReply[0] = 0; // Last one's been copied to jsonResponse; so set up for the next
    JsonReport(true, request);
    return;
  }
  
  if(StringStartsWith(request, "gcode"))
  {
    LoadGcodeBuffer(&clientQualifier[6], true);
    snprintf(scratchString, STRING_LENGTH, "{\"buff\":%u}", GetReportedGcodeBufferSpace());
   	strncat(jsonResponse, scratchString, STRING_LENGTH);
    JsonReport(true, request);
    return;
  }
  
  if(StringStartsWith(request, "files"))
  {
    char* fileList = platform->GetMassStorage()->FileList(platform->GetGCodeDir(), false);
    strncpy(jsonResponse, "{\"files\":[", STRING_LENGTH);
    strncat(jsonResponse, fileList, STRING_LENGTH);
    strncat(jsonResponse, "]}", STRING_LENGTH);
    JsonReport(true, request);
    return;
  }
  

  
  if(StringStartsWith(request, "axes"))
  {
    strncpy(jsonResponse, "{\"axes\":[", STRING_LENGTH);
    for(int8_t drive = 0; drive < AXES; drive++)
    {
      strncat(jsonResponse, "\"", STRING_LENGTH);
      snprintf(scratchString, STRING_LENGTH, "%.2f", platform->AxisLength(drive));
      strncat(jsonResponse, scratchString, STRING_LENGTH);
      if(drive < AXES-1)
        strncat(jsonResponse, "\",", STRING_LENGTH);
      else
        strncat(jsonResponse, "\"", STRING_LENGTH);
    }
    strncat(jsonResponse, "]}", STRING_LENGTH);
    JsonReport(true, request);
    return;
  }
  
  JsonReport(false, request);
  */
}


// Deal with input/output from/to the client (if any) one byte at a time.

void Panelone::Spin()
{
  if(!active)
    return;
    
  if(writing)
  {
	  if (WriteBytes())		// if we wrote something
	  {
		  platform->ClassReport("Panelone", longWait);
		  return;
	  }
  }
  
  //TODO: will need reworking with the panel refresh job
  /*
  if(platform->GetNetwork()->Active())
  {
	  for(uint8_t i = 0;
		   i < 16 && (platform->GetNetwork()->Status() & (clientConnected | byteAvailable)) == (clientConnected | byteAvailable);
	  	     ++i)
	  {
		  char c;
		  platform->GetNetwork()->Read(c);
		  //SerialUSB.print(c);

		  if(receivingPost && postFile != NULL)
		  {
			  if(MatchBoundary(c))
			  {
				  //Serial.println("Got to end of file.");
				  //postFile->Close();
				  //SendFile(clientRequest);
				  //clientRequest[0] = 0;
				  InitialisePost();
			  }
			  platform->ClassReport("Panelone", longWait);
			  return;
		  }

		  if (CharFromClient(c))
			  break;	// break if we did more than just store the character
	  }
  }
   
  if (platform->GetNetwork()->Status() & clientLive)
  {
    if(needToCloseClient)
    {
      if(platform->Time() - clientCloseTime < CLIENT_CLOSE_DELAY)
      {
    	platform->ClassReport("Panelone", longWait);
        return;
      }
      needToCloseClient = false;  
      platform->GetNetwork()->Close();
    }   
  }
  */
  platform->ClassReport("Panelone", longWait);
}

//******************************************************************************************

// Constructor and initialisation

Panelone::Panelone(Platform* p)
{ 
  platform = p;
  active = false;
  gotPassword = false;
}

void Panelone::Init()
{
  writing = false;
  receivingPost = false;
  postSeen = false;
  getSeen = false;
  jsonPointer = -1;
  clientLineIsBlank = true;
  needToCloseClient = false;
  clientLinePointer = 0;

  SetPassword(DEFAULT_PASSWORD);
  SetName(DEFAULT_NAME);
  //gotPassword = false;
  gcodeReadIndex = gcodeWriteIndex = 0;
  InitialisePost();
  lastTime = platform->Time();
  longWait = lastTime;
  active = true;
  seq = 0;
  webDebug = false;
}


void Panelone::Exit()
{
  platform->Message(HOST_MESSAGE, "Panelone class exited.\n");
  active = false;
}

void Panelone::Diagnostics()
{
  platform->Message(HOST_MESSAGE, "Panelone Diagnostics:\n");
}


// Get the actual amount of gcode buffer space we have
unsigned int Panelone::GetGcodeBufferSpace() const
{
	return (gcodeReadIndex - gcodeWriteIndex - 1u) % gcodeBufLength;
}

// Get the amount of gcode buffer space we are going to report
unsigned int Panelone::GetReportedGcodeBufferSpace() const
{
	unsigned int temp = GetGcodeBufferSpace();
	return (temp > maxReportedFreeBuf) ? maxReportedFreeBuf
			: (temp < minReportedFreeBuf) ? 0
				: temp;
}

void Panelone::WebDebug(bool wdb)
{
	webDebug = wdb;
}


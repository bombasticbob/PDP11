//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                 _              _                                         //
//              __| |  ___   ___ | |_  __ _  _ __    ___     ___            //
//             / _` | / _ \ / __|| __|/ _` || '_ \  / _ \   / __|           //
//            | (_| ||  __/| (__ | |_| (_| || |_) ||  __/ _| (__            //
//             \__,_| \___| \___| \__|\__,_|| .__/  \___|(_)\___|           //
//                                          |_|                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//            Copyright (c) 2019 by Bob Frazier and S.F.T. Inc.             //
//  Use, copying, and distribution of this software are licensed according  //
//  to the GPLv2, LGPLv2, or MIT-like license, as appropriate. See the      //
//  included 'LICENSE' and 'COPYING' files for more information.            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


// this program reads/writes FSM formatted magtape data

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> /* stdarg to make sure I have va_list and other important stuff */
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>



typedef union _RT11_VOL_HEADER_
{
  char buf[512]; // raw buffer

  struct
  {
    char    label_identifier[3];    // VOL
    char    label_number;           // typically '1'
    char    volume_identifier[6];   // volume name, 'RT11A ' typical, pad with ' '
    char    accessibility;          // ' '
    char    reserved26[26];         // 26 spaces
    char    owner_identifier[3];    // 'D%B' means tape written by DEC PDP-11
    char    owner_name[10];         // typically 10 spaces
    uint8_t DEC_standard_version;   // 1
    char    reserved28[28];         // 28 spaces
    uint8_t label_standard_version; // 3

    // rest of it is garbage
  } __attribute__((__packed__));

} RT11_VOL_HEADER;

#define SIZEOF_STRUCT_MEMBER(X,Y) sizeof(((X *)(0))->Y)

typedef union _RT11_FILE_HEADER_
{
  char buf[512]; // raw buffer

  struct
  {
    char    label_identifier[3];    // HDR
    char    label_number;           // typically '1'
    char    file_identifier[17];    // 6.3 file name, left justified, padded with ' '
    char    file_set_identifier[6]; // 'RT11A '
    char    file_section_number[4]; // '0001'
    char    file_sequence_number[4];// '0001' for first file, sequences upward
    char    generation_number[4];   // '0001'
    char    generation_version[2];  // '00'
    char    creation_date[6];       // ' YYddd' where YY is year (90) and 'ddd' is day in year (032) for example 2/1/1990
    char    expiration_date[6];     // ' 00000' indicates an expired file
    char    accessibility;          // ' '
    char    block_count[6];         // '000000'
    char    system_code[13];        // 'DECRT11A     '
    char    reserved7[7];           // 7 spaces

    // rest of it is garbage
  } __attribute__((__packed__));

} RT11_FILE_HEADER;

typedef union _RT11_FILE_EOF_
{
  char buf[512]; // raw buffer

  struct
  {
    char    label_identifier[3];    // EOF
    char    label_number;           // typically '1'
    char    file_identifier[17];    // 6.3 file name, left justified, padded with ' '
    char    file_set_identifier[6]; // 'RT11A '
    char    file_section_number[4]; // '0001'
    char    file_sequence_number[4];// '0001' for first file, sequences upward
    char    generation_number[4];   // '0001'
    char    generation_version[2];  // '00'
    char    creation_date[6];       // ' YYddd' where YY is year (90) and 'ddd' is day in year (032) for example 2/1/1990
    char    expiration_date[6];     // ' 00000' indicates an expired file
    char    accessibility;          // ' '
    char    block_count[6];         // 'nnnnnn' = # of 512 byte data blocks since the HDR record (0 if SPFUN used)
    char    system_code[13];        // 'DECRT11A     '
    char    reserved7[7];           // 7 spaces

    // rest of it is garbage
  } __attribute__((__packed__));

} RT11_FILE_EOF;

// TAPE FORMAT:
//
// * MARKER *
// VOLUME HEADER
// * MARKER*
//
// * MARKER *
// first file header
// * MARKER *
//
// \x00\x00\x00\x00 <-- data marker (?)
//
// * MARKER *
// data block (512 bytes)
// * MARKER *
//
// * MARKER *
// data block (512 bytes)
// * MARKER *
//
// ...  (tail end of file is padded with zero bytes)
//
// \x00\x00\x00\x00 <-- data marker (?)
//
// * MARKER *
// first file EOF
// * MARKER *
//
// \x00\x00\x00\x00 <-- data marker (?)
//
// (RINSE AND REPEAT FILE SEQUENCE FOR NEXT FILE)
//
// \x00\x00\x00\x00 <-- data marker (?)
//
// more zero bytes (end of tape)




uint8_t DATA_MARKER[4]={0, 0, 0, 0};
uint8_t TAPE_MARKER[4]={0, 2, 0, 0};

int iVerbosity = 0; // debug output

int read_the_tape(FILE *pTape, const char *szTapeFileName, const char *pOutPath, int bDirectory, int bOverwrite, int bConfirm, int bValidate);
int read_tape_block(FILE *pTape, void *pHeader); // always 512 byte blocks plus leading/trailing 'TAPE_MARKER'
int do_initialize_tape(const char *szFileName, int iDriveSize, int bOverwrite, const char *pLabel);

int QueryYesNo(const char *szMessage); // returns non-zero for yes, zero for no

// file utilities (some are derived code from 'ForkMe')
int FileExists(const char *szFileName);
int IsDirectory(const char *szFileName);
void *WBAllocDirectoryList(const char *szDirSpec);
void WBDestroyDirectoryList(void *pDirectoryList);
int WBNextDirectoryEntry(void *pDirectoryList, char *szNameReturn, int cbNameReturn, unsigned long *pdwModeAttrReturn);

const char *date_string(const char *pDate);

void usage()
{
  fputs("USAGE:\n"
        "  dectape -h\n"
        "  dectape [-v] tapefile\n"
        "  dectape tapefile directory\n"
        "  dectape directory tapefile\n"
        " where\n"
        " tapefile  a file that is (or will be) attached to a TMx device\n"
        " directory a directory to/from which to write files\n"
        " -h        prints this 'usage' message\n"
        " -v        Verbosity level (multiple -v to increase it)\n"
        " -n        do not overwrite existing files (the default)\n"
        " -o        DO overwrite existing files\n"
        " -q        Do not prompt to overwrite files or create a directory\n"
        " -V        validates a tape (rather than printing the directory)\n"
        " -A        append to the tape, rather than overwriting\n"
        "           (this can put duplicate file names on the tape)\n"
        " -I        Initialize a new tape file\n"
        " -S        Specify the size for a new tape file (in MB)\n"
        " -L        Specify the label for a new tape file\n"
        "\n"
        "To list the file directory of a tape, use\n"
        "    dectape tapefile\n"
        "\n"
        "To copy the tape files to a directory, use\n"
        "    dectape tapefile directory\n"
        "\n"
        "To copy a directory to a tape file, use\n"
        "    dectape directory tapefile\n"
        "\n"
        "This program is supposed to be simple.  No complaints.\n\n",
        stderr);
}

static char szTapeLabel[SIZEOF_STRUCT_MEMBER(RT11_VOL_HEADER,owner_name)] =
  { 'd','e','c','t','a','p','e',' ',' ',' ' };

int main(int argc, char *argv[])
{
FILE *pTape = NULL;
char *p1, *pEnd;
const char *p1C;
int i1, iRval;
int bDirectory = 1;
int bOverwrite = 0;
int bAppend = 0;
int bConfirm = 1;
int bValidate = 0;
int bInitialize = 0;
int iDriveSize = 32;


  // demo - do a directory of the tape

  // get options using 'getopt' so it behaves like every OTHER 'POSIX' utility
  while((i1 = getopt(argc, argv, "hvVqonAIS:L:"))
        != -1)
  {
    switch(i1)
    {
      case 'h':
        usage();
        exit(1);

      case 'v':
        iVerbosity++;
        break;

      case 'V':
        bValidate = 1;
        break;

      case 'q':
        bConfirm = 1;
        break;

      case 'n':
        bOverwrite = 0;
        break;

      case 'o':
        bOverwrite = 1;
        break;

      case 'A':
        bAppend = 1;
        break;

      case 'I':
        bInitialize = 1;
        break;

      case 'S':
        iDriveSize = atoi(optarg);
        break;

      case 'L':
        pEnd = &(szTapeLabel[sizeof(szTapeLabel)]);

        for(p1C=optarg, p1=szTapeLabel; *p1C && p1 < pEnd; )
        {
          *(p1++) = *(p1C++);
        }

        // pad with spaces
        while(p1 < pEnd)
        {
          *(p1++) = ' ';
        }

        break;
    }
  }

  argc -= optind;
  argv += optind;

  if(argc < 1)
  {
    usage();
    exit(1);
  }

  // see if one is a file, and one is a directory
  // if output file, make sure it does not exist unless 'overwrite'
  // if output dir, should it be empty??

  if(argc >= 2)
  {
    if(bInitialize)
    {
      fprintf(stderr, "Initialize does not take 2 parameters\n");
      usage();
      exit(1);
    }

    bDirectory = 0;

    // TODO:  if first param is a directory, and 2nd is a tape file
    //        zero out the tape file and write directory contents to it
    //        after getting confirmation [just in case].  I could verify
    //        that it either contains zeros or that it already has a tape
    //        header in it [then duplicate the tape header info]

    fprintf(stderr, "temporary, copying not supported yet\n");
    exit(1);
  }

  if(bInitialize)
  {
    // TODO:  warn if unnecessary params specified??

    return do_initialize_tape(argv[0], iDriveSize, bOverwrite, szTapeLabel);
  }

  // LIST TAPE DIRECTORY, VALIDATE, OR COPY TAPE TO DIRECTORY

  pTape = fopen(argv[0], "r");
  if(!pTape)
  {
    fprintf(stderr, "Unable to open tape file \"%s\"\n", argv[0]);
    exit(2);
  }

  iRval = read_the_tape(pTape, argv[0], (const char *)(bDirectory ? NULL : argv[1]),
                        bDirectory, bOverwrite, bConfirm, bValidate);

//exit_point:
  fclose(pTape);
  return iRval;
}

int read_tape_block(FILE *pTape, void *pHeader)
{
uint8_t marker[4];  // where to read the markers into
size_t sRval;

  if(!pTape || feof(pTape))
    return -1;

  if(fread(marker, sizeof(marker), 1, pTape) != 1)
    return -2;

  if(!memcmp(marker, DATA_MARKER, sizeof(marker))) // end of tape
    return 1; // this is acceptable, but "an error" if I don't expect it

  if(memcmp(marker, TAPE_MARKER, sizeof(marker)))
    return -3;

  if(feof(pTape) || fread(pHeader, 512, 1, pTape) != 1)
    return -4;

  if(feof(pTape) || fread(marker, sizeof(marker), 1, pTape) != 1)
    return -5;

  if(memcmp(marker, TAPE_MARKER, sizeof(marker)))
    return -6;

  return 0; // OK!
}

int read_the_tape(FILE *pTape, const char *szTapeFileName, const char *pOutPath, int bDirectory, int bOverwrite, int bConfirm, int bValidate)
{
int iRval = -1, i1, nBlocks, iSeq;
size_t lPos;
RT11_VOL_HEADER vol;
RT11_FILE_HEADER file;
RT11_FILE_EOF eof;
uint8_t data[512]; // file data
uint8_t marker[4];  // where to read the markers into
char tbuf[512];


  iSeq = 0;

  if(read_tape_block(pTape, &vol)) // read volume info
  {
    fprintf(stderr, "Unable to read header on tape file \"%s\"\n", szTapeFileName);
    return -9;
  }

  if(!memcmp(vol.label_identifier, "HDR", 3))
  {
    memcpy(&file, &vol, sizeof(file));

    goto do_file;
  }

  if(memcmp(vol.label_identifier, "VOL", 3) || vol.label_number != '1')
  {
    fprintf(stderr, "Bad volume header - \"%-3.3s\" '%c'\n",
            vol.label_identifier, vol.label_number);

    return -8;
  }

  if(bDirectory || bValidate)
  {
    printf("RT11 TAPE  '%-3.3s' '%-10.10s' V%c Label V%c\n",
           vol.owner_identifier, vol.owner_name,
           vol.DEC_standard_version, vol.label_standard_version);
  }
  else if(bValidate)
  {
    fputs("** NO TAPE HEADER **\n", stdout);
  }

  while(!feof(pTape))
  {
    i1 = read_tape_block(pTape, &file); // read volume info

    if(i1 > 0)
    {
      if(bDirectory && !bValidate)
      {
        printf("\nEND OF TAPE\n\n");
      }

      break;
    }
    else if(i1 < 0 || memcmp(file.label_identifier, "HDR", 3) || file.label_number != '1')
    {
      if(i1 < 0)
        fprintf(stderr, "Unable to read file header at position %ld\n", (long)ftell(pTape) - 4);
      else
        fprintf(stderr, "Invalid file header at position %ld - \"%-3.3s\" '%c'\n",
                (long)ftell(pTape) - 4,
                file.label_identifier, file.label_number);

      return -10;
    }

do_file:
    iSeq++;

    memset(tbuf, 0, sizeof(tbuf));
    memcpy(tbuf, file.file_sequence_number, sizeof(file.file_sequence_number));
    if(iSeq != atoi(tbuf))
    {
      fprintf(stderr, "WARNING: Invalid file seq number in header - %d vs %d\n",
              iSeq, atoi(tbuf));
    }

    if(iSeq == 1 && bDirectory && !bValidate)
    {
      fputs("  FILE NAME         CREATE DATE  BLOCKS  TOTAL BYTES\n"
            "  ================  ===========  ======  ===========\n", stdout);
    }

    if(fread(marker, sizeof(marker), 1, pTape) != 1 ||
       memcmp(marker, DATA_MARKER, sizeof(marker))) // need a marker here
    {
      fprintf(stderr, "missing data marker at position %ld, file \"%-17.17s\"\n",
              (long)ftell(pTape) - 4,
              file.file_identifier);

      return -11;
    }

    nBlocks = 0;

    while(!feof(pTape))
    {
      lPos = ftell(pTape); // current position

      i1 = read_tape_block(pTape, data); // read file data block
      // see if it's an EOF header.  Yes, this is a LAME way of doing it, but that's
      // the way this thing works.

      if(i1 > 0) // a read error (TODO:  should >0 be used instead of 'above' sequence?
      {
        break; // end of file
      }
      else if(i1 < 0)
      {
        fprintf(stderr, "read error at position %ld, file \"%-17.17s\"\n",
                (long)lPos, file.file_identifier);

        return -7;
      }

      // TODO:  write the actual data to an output file
      nBlocks++;
    }

    if(bDirectory && !bValidate)
    {
      // directory output
      printf("  %-17.17s  %-9.9s  %6d  %11ld\n",
             file.file_identifier,
             date_string(file.creation_date),
             nBlocks, (long)(nBlocks * 512));
    }

    i1 = read_tape_block(pTape, &eof); // read file EOF block

    if(i1 > 0)
    {
      printf("unexpected (missing EOF record)\nEND OF TAPE\n\n");

      return 1;
    }
    else if(i1 < 0 || memcmp(eof.label_identifier, "EOF", 3) || eof.label_number != '1')
    {
      if(i1 < 0)
        fprintf(stderr, "Unable to read EOF header at position %ld\n", lPos);
      else
        fprintf(stderr, "Invalid EOF header at position %ld - \"%-3.3s\" '%c'\n",
                lPos, eof.label_identifier, eof.label_number);

      return -12;
    }

    memset(tbuf, 0, sizeof(tbuf));
    memcpy(tbuf, eof.block_count, sizeof(eof.block_count)); // make sure tbuf is big enough

    if(memcmp(eof.file_identifier, file.file_identifier, sizeof(eof.file_identifier)) ||
       atoi(tbuf) != nBlocks)
    {
      printf("    *EOF HEADER MISMATCH* \"%-17.17s\"  %s\n",
             eof.file_identifier, tbuf);
    }

    // there should be a data marker now

    if(fread(marker, sizeof(marker), 1, pTape) != 1)
    {
      fprintf(stderr, "missing data/tape marker at position %ld, file \"%-17.17s\"\n",
              (long)ftell(pTape) - 4,
              file.file_identifier);

      return -13;
    }

    if(memcmp(marker, DATA_MARKER, sizeof(marker))) // data marker means end of file should be next
    {
      printf("missing data marker at end of EOF record\n");

      return -14;
    }
  }

  if(bValidate)
    fputs("** TAPE VALIDATED **\n", stdout);

  return 0; // won't get here but do this anyway
}

int QueryYesNo(const char *szMessage)
{
char tbuf[256];
int i1;

  while(!feof(stdin) && !ferror(stdin))
  {
    fprintf(stderr, "%s (y/n)?", szMessage);
    fgets(tbuf, sizeof(tbuf), stdin);

    // trim any trailing white space
    while((i1 = strlen(tbuf)) > 0 && tbuf[i1 - 1] <= ' ')
      tbuf[i1 - 1] = 0;

    // trim any lead white space
    while((i1 = strlen(tbuf)) > 0 && tbuf[0] <= ' ')
    {
      if(i1 > 1)
        memmove(tbuf, tbuf + 1, i1 - 1);

      tbuf[i1 - 1] = 0;
    }

    if(tbuf[0] == 'Y' || tbuf[0] == 'y')
      return 1;

    if(tbuf[0] == 'n' || tbuf[0] == 'N')
      return 0;

    fprintf(stderr, "Please respond with 'Y' or 'N'\n");
  }

  return 0; // for now (probably won't get here)
}

int do_initialize_tape(const char *szFileName, int iDriveSize, int bOverwrite, const char *pLabel)
{
int i1;
long lFileSize = 0, lTargetSize = 1024L * 1024L * iDriveSize;
FILE *pTape;
RT11_VOL_HEADER *pHdr;
char buf[512]; // what I write from


  if(!pLabel || !*pLabel) // would be padded with white space if done right
    pLabel = "dectape   ";

  if(iDriveSize <= 0)
  {
    fprintf(stderr, "Invalid drive size %d\n", iDriveSize);

    usage();

    return -1;
  }

  // STEP 1:  does the file exist??

  if(FileExists(szFileName))
  {
    if(!bOverwrite && !QueryYesNo("Overwrite existing file"))
      return 1; // you said 'no'

    // make sure file is zero bytes long

    if(truncate(szFileName, 0)) // zero bytes long
    {
      fprintf(stderr, "Unable to write (truncate) to \"%s\", errno=%d (%xH)\n",
              szFileName, errno, errno);

      return -1;
    }
  }

  pTape = fopen(szFileName, "w");
  if(!pTape)
  {
    fprintf(stderr, "Unable to open \"%s\", errno=%d (%xH)\n",
            szFileName, errno, errno);

    return -1;
  }

  // build a header

  memset(buf, ' ', sizeof(buf)); // rather than zeros, use white space (no harm)

  pHdr = (RT11_VOL_HEADER *)buf;

  memcpy(pHdr->label_identifier, "VOL", sizeof(pHdr->label_identifier));
  pHdr->label_number = '1';
  memcpy(pHdr->volume_identifier, "RT11A ", sizeof(pHdr->volume_identifier));
  pHdr->accessibility = ' ';
  memset(pHdr->reserved26, ' ', sizeof(pHdr->reserved26));
  memcpy(pHdr->owner_identifier, "D%B", sizeof(pHdr->owner_identifier));
  memcpy(pHdr->owner_name, pLabel, sizeof(pHdr->owner_name));
  pHdr->DEC_standard_version = '1';
  memset(pHdr->reserved28, ' ', sizeof(pHdr->reserved28));
  pHdr->label_standard_version = '3';

  // tape marker, header, tape marker

  if((i1 = fwrite("\x00\x02\x00\x00", 4, 1, pTape)) != 1 ||
     (i1 = fwrite(buf, sizeof(buf), 1, pTape)) != 1 ||
     (i1 = fwrite("\x00\x02\x00\x00", 4, 1, pTape)) != 1)
  {
write_error:
    fprintf(stderr, "ERROR:  fwrite() returns %d, errno=%d (%xH)\n",
            i1, errno, errno);

    fclose(pTape);
    return -1;
  }

  // always write at least one empty buffer minus the size of the marker * 2

  memset(buf, 0, sizeof(buf));

  if((i1 = fwrite(buf, sizeof(buf) - 8, 1, pTape)) != 1)
    goto write_error;

  lFileSize = sizeof(buf) * 2; // tape and marker and remaining 0 bytes

  while(lFileSize < lTargetSize)
  {
    if((i1 = fwrite(buf, sizeof(buf), 1, pTape)) != 1)
      goto write_error;

    lFileSize += sizeof(buf);
  }

  fclose(pTape);

  return 0;
}



const char *date_string(const char *pDate)
{
int iYear, iDay, iMonth;
const short *pD;
static char szRval[16];
static const char * const aszMonthNames[13]=
{
  "???","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
static const short aDays[13]=
{
  1,
  1 + 31,
  1 + 31 + 28,
  1 + 31 + 28 + 31,
  1 + 31 + 28 + 31 + 30,
  1 + 31 + 28 + 31 + 30 + 31,
  1 + 31 + 28 + 31 + 30 + 31 + 30,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
  1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
};
static const short aDaysLeap[13]=
{
  1,
  1 + 31,
  1 + 31 + 29,
  1 + 31 + 29 + 31,
  1 + 31 + 29 + 31 + 30,
  1 + 31 + 29 + 31 + 30 + 31,
  1 + 31 + 29 + 31 + 30 + 31 + 30,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
  1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
};


  // ignore first character if it's a space, otherwise # of years since 1900
  // format the date from the following information:
  //
  // YYYddd  where 'YYY' is years since 1900, and 'ddd' is days since Jan 1 (inclusive)

  if(*pDate >= '0' && *pDate <= '9')
    iYear = 1900 + 100 * (*pDate - '0');
  else
    iYear = 1900;

  iYear += 10 * (pDate[1] - '0') + (pDate[2] - '0');

  iDay = 100 * (pDate[3] - '0')
       + 10 * (pDate[4] - '0')
       + (pDate[5] - '0');

  if((iYear % 400) == 0 ||
     ((iYear % 4) == 0 && (iYear % 100) != 0)) // leap year
  {
    pD = aDaysLeap;
  }
  else
  {
    pD = aDays;
  }

  for(iMonth=1; iMonth < 12; iMonth++)
  {
    if(iDay < pD[iMonth])
      break;
  }

  iDay -= pD[iMonth - 1];

  snprintf(szRval, sizeof(szRval), "%02d-%s-%02d",
           (iDay % 100),
           aszMonthNames[iMonth],
           (iYear % 100));

  return szRval;
}


// FILE UTILITIES
// Some of these were derived from 'ForkMe' - http://github.com/bombasticbob/ForkMe
// that utility is covered by the same type of license, and was written by the same author as 'dectape'

#define WBAlloc(X) malloc(X)
#define WBReAlloc(X,Y) realloc(X,Y)
#define WBFree(X) free(X)

int FileExists(const char *szFileName)
{
struct stat sb;

  return !stat(szFileName, &sb);
}

int IsDirectory(const char *szFileName)
{
struct stat sb;

  // NOTE:  this returns info about symlink targets if 'szFileName' is a symlink

  return !stat(szFileName, &sb) &&
         S_ISDIR(sb.st_mode);
}

typedef struct __DIRLIST__
{
  const char *szPath, *szNameSpec;
  DIR *hD;
  struct stat sF;
  union
  {
    char cde[sizeof(struct dirent) + NAME_MAX + 2];
    struct dirent de;
  };
// actual composite 'search name' follows
} DIRLIST;

void *WBAllocDirectoryList(const char *szDirSpec)
{
DIRLIST *pRval;
char *p1, *p2;
int iLen, nMaxLen;
char *pBuf;

  if(!szDirSpec || !*szDirSpec)
  {
//    WB_WARN_PRINT("WARNING - %s - invalid directory (NULL or empty)\n", __FUNCTION__);
    return NULL;
  }

  iLen = strlen(szDirSpec);
  nMaxLen = iLen + 32;

  pBuf = WBAlloc(nMaxLen);
  if(!pBuf)
  {
//    WB_ERROR_PRINT("ERROR - %s - Unable to allocate memory for buffer size %d\n", __FUNCTION__, nMaxLen);
    return NULL;
  }

  if(szDirSpec[0] == '/') // path starts from the root
  {
    memcpy(pBuf, szDirSpec, iLen + 1);
  }
  else // for now, force a path of './' to be prepended to path spec
  {
    pBuf[0] = '.';
    pBuf[1] = '/';

    memcpy(pBuf + 2, szDirSpec, iLen + 1);
    iLen += 2;
  }

  // do a reverse scan until I find a '/'
  p1 = ((char *)pBuf) + iLen;
  while(p1 > pBuf && *(p1 - 1) != '/')
  {
    p1--;
  }

//  WB_ERROR_PRINT("TEMPORARY - \"%s\" \"%s\" \"%s\"\n", pBuf, p1, szDirSpec);

  if(p1 > pBuf)
  {
    // found, and p1 points PAST the '/'.  See if it ends in '/' or if there are wildcards present
    if(!*p1) // name ends in '/'
    {
      if(p1 == (pBuf + 1) && *pBuf == '/') // root dir
      {
        p1++;
      }
      else
      {
        *(p1 - 1) = 0;  // trim the final '/'
      }

      p1[0] = '*';
      p1[1] = 0;
    }
    else if(strchr(p1, '*') || strchr(p1, '?'))
    {
      if(p1 == (pBuf + 1) && *pBuf == '/') // root dir
      {
        memmove(p1 + 1, p1, strlen(p1) + 1);
        *(p1++) = 0; // after this, p1 points to the file spec
      }
      else
      {
        *(p1 - 1) = 0;  // p1 points to the file spec
      }
    }
    else if(IsDirectory(pBuf)) // entire name is a directory
    {
      // NOTE:  root directory should NEVER end up here

      p1 += strlen(p1);
      *(p1++) = 0; // end of path (would be '/')
      p1[0] = '*';
      p1[1] = 0;
    }
//    else
//    {
//      WB_WARN_PRINT("TEMPORARY:  I am confused, %s %s\n", pBuf, p1);
//    }
  }
  else
  {
    // this should never happen if I'm always prepending a './'
    // TODO:  make this more consistent, maybe absolute path?

//    WB_WARN_PRINT("TEMPORARY:  should not happen, %s %s\n", pBuf, p1);

    if(strchr(pBuf, '*') || strchr(pBuf, '?')) // wildcard spec
    {
      p1 = (char *)pBuf + 1; // make room for zero byte preceding dir spec
      memmove(pBuf, p1, iLen + 1);
      *pBuf = 0;  // since it's the current working dir just make it a zero byte (empty string)
    }
    else if(IsDirectory(pBuf))
    {
      p1 = (char *)pBuf + iLen;
      *(p1++) = 0; // end of path (would be '/')
      p1[0] = '*';
      p1[1] = 0;
    }
  }

  pRval = WBAlloc(sizeof(DIRLIST) + iLen + strlen(p1) + 2);

  if(pRval)
  {
    pRval->szPath = pBuf;
    pRval->szNameSpec = p1;

    p2 = (char *)(pRval + 1);
    strcpy(p2, pBuf);
    p2 += strlen(p2);
    *(p2++) = '/';
    strcpy(p2, p1);
    p1 = (char *)(pRval + 1);

    pRval->hD = opendir(pBuf);

//    WB_ERROR_PRINT("TEMPORARY - opendir for %s returns %p\n", pBuf, pRval->hD);

    if(pRval->hD == NULL)
    {
//      WB_WARN_PRINT("WARNING - %s - Unable to open dir \"%s\", errno=%d\n", __FUNCTION__, pBuf, errno);

      WBFree(pBuf);
      WBFree(pRval);

      pRval = NULL;
    }
  }
  else
  {
//    WB_ERROR_PRINT("ERROR - %s - Unable to allocate memory for DIRLIST\n", __FUNCTION__);
    WBFree(pBuf);  // no need to keep this around
  }

  return pRval;
}

void WBDestroyDirectoryList(void *pDirectoryList)
{
  if(pDirectoryList)
  {
    DIRLIST *pD = (DIRLIST *)pDirectoryList;

    if(pD->hD)
    {
      closedir(pD->hD);
    }
    if(pD->szPath)
    {
      WBFree((void *)(pD->szPath));
    }

    WBFree(pDirectoryList);
  }
}

// returns < 0 on error, > 0 on EOF, 0 for "found something"

int WBNextDirectoryEntry(void *pDirectoryList, char *szNameReturn,
                         int cbNameReturn, unsigned long *pdwModeAttrReturn)
{
struct dirent *pD;
struct stat sF;
char *p1, *pBuf;
//static char *p2; // temporary
int iRval = 1;  // default 'EOF'
DIRLIST *pDL = (DIRLIST *)pDirectoryList;


  if(!pDirectoryList)
  {
    return -1;
  }

  // TODO:  improve this, maybe cache buffer or string length...
  pBuf = WBAlloc(strlen(pDL->szPath) + 8 + NAME_MAX);

  if(!pBuf)
  {
    return -2;
  }

  strcpy(pBuf, pDL->szPath);
  p1 = pBuf + strlen(pBuf);
  if(p1 > pBuf && *(p1 - 1) != '/') // it does not already end in /
  {
    *(p1++) = '/';  // for now assume this
    *p1 = 0;  // by convention
  }

  if(pDL->hD)
  {
    while((pD = readdir(pDL->hD))
          != NULL)
    {
      // skip '.' and '..'
      if(pD->d_name[0] == '.' &&
         (!pD->d_name[1] ||
          (pD->d_name[1] == '.' && !pD->d_name[2])))
      {
//        WB_ERROR_PRINT("TEMPORARY:  skipping %s\n", pD->d_name);
        continue;  // no '.' or '..'
      }

      strcpy(p1, pD->d_name);

      if(!lstat(pBuf, &sF)) // 'lstat' returns data about a file, and if it's a symlink, returns info about the link itself
      {
        if(!fnmatch(pDL->szNameSpec, p1, 0/*FNM_PERIOD*/))  // 'tbuf2' is my pattern
        {
          iRval = 0;

          if(pdwModeAttrReturn)
          {
            *pdwModeAttrReturn = sF.st_mode;
          }

          if(szNameReturn && cbNameReturn > 0)
          {
            strncpy(szNameReturn, p1, cbNameReturn);
          }

          break;
        }
//        else
//        {
//          p2 = pDL->szNameSpec;
//
//          WB_ERROR_PRINT("TEMPORARY:  \"%s\" does not match \"%s\"\n", p1, p2);
//        }
      }
//      else
//      {
//        WB_WARN_PRINT("%s: can't 'stat' %s, errno=%d (%08xH)\n", __FUNCTION__, pBuf, errno, errno);
//      }
    }
  }

  if(pBuf)
  {
    WBFree(pBuf);
  }

  return iRval;

}


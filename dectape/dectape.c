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
//          Copyright (c) 2019 by S.F.T. Inc. - All rights reserved         //
//  Use, copying, and distribution of this software are licensed according  //
//  to the GPLv2, LGPLv2, or BSD license, as appropriate. See the included  //
//  'README.dectape.txt' file for more information.                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


// this program reads/writes FSM formatted magtape data

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


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

int main(int argc, char *argv[])
{
FILE *pTape = NULL;
int i1, iRval;
int bDirectory = 1;
int bOverwrite = 0;
int bConfirm = 1;
int bValidate = 0;


  // demo - do a directory of the tape

  // get options using 'getopt' so it behaves like every OTHER 'POSIX' utility
  while((i1 = getopt(argc, argv, "hvVqon"))
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
    bDirectory = 0;

    // TODO:  if first param is a directory, and 2nd is a tape file
    //        zero out the tape file and write directory contents to it
    //        after getting confirmation [just in case].  I could verify
    //        that it either contains zeros or that it already has a tape
    //        header in it [then duplicate the tape header info]

    fprintf(stderr, "temporary, copying not supported yet\n");
    exit(1);
  }

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


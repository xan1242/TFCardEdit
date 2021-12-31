// Yu-Gi-Oh! Tag Force CardInfo Tool
// by Xan

#include <iostream>
#include <algorithm>
#include <string>
using namespace std;

#define TF1_MINCARD_INTID 4000
#define TF1_MINCARD_ID 4007
#define TF1_MAXCARD_ID 6958

#define TFCD_HELPMESSAGE "\
USAGE (read mode): %s InFolder [OutFilename] [Language]\n\
USAGE (write mode): %s -w InFilename [OutFolder] [Language]\n\
\n\
READ MODE ARGUMENTS:\n\
Folder - Must contain the following files:\n\
CARD_IntID.bin\n\
CARD_Pass.bin\n\
CARD_Prop.bin\n\
CARD_Indx_LANG.bin\n\
CARD_Name_LANG.bin\n\
CARD_Desc_LANG.bin\n\
\n\
Where LANG can be: J, E, G, F, I or S\n\
\n\
OutFilename - Filename of the output text database. If undefined, it's autogenerated from the folder name.\n\
Language - Sets the LANG extension in the filenames above (default: E)\n\
\n\
For write mode help, use -w without arguments.\n"

#define TFCD_HELPMESSAGE_WRITEMODE "\
USAGE (write mode): %s -w InFilename [OutFolder] [Language]\n\
\n\
WRITE MODE ARGUMENTS:\n\
InFilename - Input card database text file (which is exported from this utility)\n\
OutFolder - Name of the output folder, which will export the following files:\n\
CARD_IntID.bin\n\
CARD_Pass.bin\n\
CARD_Prop.bin\n\
CARD_Indx_LANG.bin\n\
CARD_Name_LANG.bin\n\
CARD_Desc_LANG.bin\n\
\n\
Where LANG can be: J, E, G, F, I or S\n\
\n\
If OutFolder name is undefined, it's autogenerated from the input filename.\n\
\n\
Language - Sets the LANG extension in the filenames above (default: E)\n"

#define TFCD_HELPMESSAGE_VARS argv[0], argv[0]

struct CardPair
{
    wchar_t* Name;
    wchar_t* Description;
}*CardIndex;

struct CardProp
{
    unsigned int Prop1;
    unsigned int Prop2;
}*CardProps;


// using TF1 as a base for now...
#define MONSTER_TYPE_COUNT 23
enum MonsterTypes
{
    None,
    Dragon,
    Zombie,
    Fiend,
    Pyro,
    SeaSerpent,
    Rock,
    Machine,
    Fish,
    Insect,
    Beast,
    BeastWarrior,
    Plant,
    Aqua,
    Warrior,
    WingedBeast,
    Fairy,
    Thunder,
    Spellcaster,
    Reptile
};

char MonsterTypeNames[MONSTER_TYPE_COUNT][16] =
{
    "None",
    "Dragon",
    "Zombie",
    "Fiend",
    "Pyro",
    "Sea Serpent",
    "Rock",
    "Machine",
    "Fish",
    "Dinosaur",
    "Insect",
    "Beast",
    "Beast-Warrior",
    "Plant",
    "Aqua",
    "Warrior",
    "Winged Beast",
    "Fairy",
    "Spellcaster",
    "Thunder",
    "Reptile",
    "Divine-Beast"
};

#define CARD_KIND_COUNT 14
enum CardKinds
{
    Normal,
    Effect,
    Fusion,
    FusionEffect,
    Ritual,
    RitualEffect,
    Toon,
    Spirit,
    Union,
    Spell = 13,
    Trap = 14
};

char CardKindNames[CARD_KIND_COUNT][16]
{
    "Normal" // never used
    "/Effect",
    "/Fusion",
    "/Fusion/Effect",
    "/Ritual",
    "/Ritual/Effect",
    "/Toon/Effect",
    "/Spirit/Effect",
    "/Union/Effect",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Spell",
    "Trap"
};

#define CARD_RARITY_COUNT 5
enum CardRarity
{
    Common,
    Rare,
    SuperRare,
    UltraRare,
    UltimateRare
};

char CardRarityNames[CARD_RARITY_COUNT][16]
{
    "Common",
    "Rare",
    "SuperRare",
    "UltraRare",
    "UltimateRare"
};

#define CARD_ATTRIBUTE_COUNT 9
enum CardAttributes
{
    LIGHT,
    DARK,
    WATER,
    FIRE,
    EARTH,
    WIND,
    DIVINE,
    SPELL,
    TRAP
};

char CardAttributeNames[CARD_ATTRIBUTE_COUNT][8]
{
    "LIGHT",
    "DARK",
    "WATER",
    "FIRE",
    "EARTH",
    "WIND",
    "DIVINE",
    "SPELL",
    "TRAP"
};

#define CARD_ICON_COUNT 6
enum CardIcons
{
    Counter,
    Field,
    Equip,
    Continuous,
    QuickPlay,
    RitualSpell
};

char CardIconNames[CARD_ICON_COUNT][16]
{
    "Counter",
    "Field",
    "Equip",
    "Continuous",
    "Quick-Play",
    "Ritual"
};

struct CardImporter
{
    unsigned int CardID;
    wchar_t* Name;
    wchar_t* Description;
    unsigned int Password;
    unsigned int ATK;
    unsigned int DEF;
    unsigned int CardExistFlag;
    unsigned int Kind;
    unsigned int Attr;
    unsigned int Level;
    unsigned int Icon;
    unsigned int Type;
    unsigned int Rarity;
}*ImportDB, NullCard = {0};

void* ImportedFile;
unsigned int ImportedFileSize;
unsigned int ImportedCardsCount;
wchar_t CardNameTempBuffer[1024];
wchar_t CardDescTempBuffer[2048];

wchar_t* StringBufferName;
wchar_t* StringBufferDesc;
unsigned int CardCount = 0;

unsigned short int* IntIDs;
unsigned int IntIDCount;
short int MinCard_IntID = TF1_MINCARD_INTID;

short int MinCard_ID = TF1_MINCARD_ID;
short int MaxCard_ID = TF1_MAXCARD_ID;

unsigned int* CardPasswords;

char Language[8];
char FilePath[1024];
char FilePath2[1024];
char FilePath3[1024];
char FolderName[1024];
wchar_t FolderNameWide[1024];
char OutTextName[1024];
struct stat st = { 0 };

bool bFileExists(const char* Filename)
{
    FILE* fcheck = fopen(Filename, "rb");
    if (!fcheck)
        return false;
    fclose(fcheck);
    return true;
}

void ReplaceCharsW(wchar_t* dest, wchar_t* source, wchar_t oldchar, wchar_t newchar)
{
    wstring CppSource(source);
    replace(CppSource.begin(), CppSource.end(), oldchar, newchar);
    wcscpy(dest, CppSource.c_str());
}

unsigned short int GetInternalID(unsigned short int CardID, unsigned short int MinCardID)
{
    if (CardID < MinCardID)
        return 0;
    return IntIDs[(CardID - MinCardID)];
}

int LoadIntIDs(const char* IntIDFilename)
{
    FILE* fintid = fopen(IntIDFilename, "rb");
    if (!fintid)
    {
        printf("ERROR: Can't open file %s for reading!\n", IntIDFilename);
        perror("ERROR");
        return -1;
    }

    if (stat(IntIDFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", IntIDFilename);
        return -1;
    }


    IntIDs = (unsigned short int*)malloc(st.st_size);
    fread(IntIDs, sizeof(char), st.st_size, fintid);

    IntIDCount = st.st_size / 2;

    fclose(fintid);
    return 0;
}

bool bCheckIfCardExists(unsigned int CardID)
{
    for (unsigned int i = 0; i < ImportedCardsCount; i++)
    {
        if (ImportDB[i].CardID == CardID)
            return true;
    }
    return false;
}

int CreateIntIDs(const char* IntIDFilename)
{
    unsigned int IntIDsize = ((MaxCard_ID - MinCard_IntID) * 2) + 2;
    unsigned int IntIDcounter = 1;

    IntIDs = (unsigned short int*)malloc(IntIDsize);
    for (unsigned int i = MinCard_IntID; i <= MaxCard_ID; i++)
    {
        if (bCheckIfCardExists(i))
        {
            IntIDs[i - MinCard_IntID] = IntIDcounter;
            IntIDcounter++;
        }
        else
            IntIDs[i - MinCard_IntID] = 0;
    }

    FILE *fintid = fopen(IntIDFilename, "wb");
    if (!fintid)
    {
        printf("ERROR: Can't open file %s for writing!\n", IntIDFilename);
        perror("ERROR");
        return -1;
    }

    fwrite(IntIDs, sizeof(char), IntIDsize, fintid);

    fclose(fintid);
    return 0;
}

int LoadCardPasswords(const char* PassFilename)
{
    FILE* fpass = fopen(PassFilename, "rb");
    if (!fpass)
    {
        printf("ERROR: Can't open file %s for reading!\n", PassFilename);
        perror("ERROR");
        return -1;
    }

    if (stat(PassFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", PassFilename);
        return -1;
    }


    CardPasswords = (unsigned int*)malloc(st.st_size);
    fread(CardPasswords, sizeof(char), st.st_size, fpass);

    fclose(fpass);
    return 0;
}

int CreateCardPasswords(const char* PassFilename)
{
    CardPasswords = (unsigned int*)calloc(ImportedCardsCount + 1, sizeof(int));
    for (unsigned int i = 0; i < ImportedCardsCount; i++)
        CardPasswords[i + 1] = ImportDB[i].Password;

    FILE* fpass = fopen(PassFilename, "wb");
    if (!fpass)
    {
        printf("ERROR: Can't open file %s for writing!\n", PassFilename);
        perror("ERROR");
        return -1;
    }

    fwrite(CardPasswords, sizeof(int), ImportedCardsCount + 1, fpass);

    fclose(fpass);
    return 0;
}

int LoadCardProp(const char* PropFilename)
{
    FILE* fprop = fopen(PropFilename, "rb");
    if (!fprop)
    {
        printf("ERROR: Can't open file %s for reading!\n", PropFilename);
        perror("ERROR");
        return -1;
    }

    if (stat(PropFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", PropFilename);
        return -1;
    }


    CardProps = (CardProp*)malloc(st.st_size);
    fread(CardProps, sizeof(char), st.st_size, fprop);

    fclose(fprop);
    return 0;
}

int CreateCardProp(const char* PropFilename)
{
    CardProps = (CardProp*)calloc(ImportedCardsCount + 1, sizeof(CardProp));

    for (unsigned int i = 0; i < ImportedCardsCount; i++)
    {
        if (ImportDB[i].ATK > 5110)
            ImportDB[i].ATK = 5110;

        if (ImportDB[i].DEF > 5110)
            ImportDB[i].DEF = 5110;

        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop1 = ImportDB[i].CardID;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop1 += (ImportDB[i].ATK / 10) << 13;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop1 += (ImportDB[i].DEF / 10) << 22;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop1 += ImportDB[i].CardExistFlag << 31;

        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 = ImportDB[i].Kind;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 += ImportDB[i].Attr << 4;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 += ImportDB[i].Level << 8;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 += ImportDB[i].Icon << 12;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 += ImportDB[i].Type << 15;
        CardProps[GetInternalID(ImportDB[i].CardID, MinCard_IntID)].Prop2 += ImportDB[i].Rarity << 20;
    }

    FILE* fprop = fopen(PropFilename, "wb");
    if (!fprop)
    {
        printf("ERROR: Can't open file %s for writing!\n", PropFilename);
        perror("ERROR");
        return -1;
    }

    fwrite(CardProps, sizeof(CardProp), ImportedCardsCount + 1, fprop);

    fclose(fprop);
    return 0;
}

/*CardProp* GetCardProp(unsigned short int CardID)
{
    return &(CardProps[GetInternalID(CardID, MinCard_ID)]);
}*/

wchar_t* CARD_GetCardName(unsigned short int CardID)
{
    return CardIndex[GetInternalID(CardID, MinCard_IntID)].Name;
}

wchar_t* CARD_GetCardDesc(unsigned short int CardID)
{
    return CardIndex[GetInternalID(CardID, MinCard_IntID)].Description;
}

unsigned int CARD_GetPassword(unsigned short int CardID)
{
    return CardPasswords[GetInternalID(CardID, MinCard_IntID)];
}

// TODO: figure out the bit magic for other games...
// POTENTIAL SUPPORT FOR LEGACY OF THE DUELIST - THIS TOOL CAN GET ALL CARD NAMES AND DESCRIPTIONS OUT OF THAT GAME
// very similar format, but the card props are different
// theory says Master Duel might also use this format...
// 
// THIS IS BASED ON TF1!
// 
// ==== Prop1 bits ==== DONE!
// 
// ATK and DEF are written divided by 10 (so ATK 3000 is 300)
// 9 bits wide means max number is 511 (so max ATK or DEF is theoretically 5110)
// 
// CardID mask: 0x1FFF (13 bits wide) - bit 0
// ATK mask: 0x3FE000 (9 bits wide) - bit 13
// DEF mask: 0x7FC00000 (9 bits wide) - bit 22
// CardExistFlag mask: 0x80000000 - bit 31
// 
// 
// ==== Prop2 bits ==== DONE?
// 
// Kind mask: 0xF (4 bits wide) - bit 0
// Attr mask: 0xF0 (4 bits wide) - bit 4
// Level mask: 0xF00 (4 bits wide) - bit 8
// Icon mask: 0x7000 (3 bits wide) - bit 12
// Type mask: 0xF8000 (5 bits wide) - bit 15
// Rarity mask: 0xF00000 (4 bits wide maybe) - bit 20
//


// Prop1 stuff
unsigned int CARD_GetAtk(unsigned short int CardID)
{
    unsigned int atk = (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop1 & 0x3FE000) >> 13;
    //if (atk == 511)
    //    return 0;
    return atk * 10;
}

unsigned int CARD_GetDef(unsigned short int CardID)
{
    unsigned int def = (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop1 & 0x7FC00000) >> 22;
   // if (def == 511)
    //    return 0;
    return def * 10;
}

unsigned int CARD_GetCardExistFlag(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop1 & 0x80000000) >> 31;
}

unsigned short int CARD_GetCardID(unsigned short int InternalID)
{
    return (CardProps[InternalID].Prop1 & 0x1FFF);
}

// Prop2 stuff
unsigned int CARD_GetKind(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0xF);
}

unsigned int CARD_GetAttr(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0xF0) >> 4;
}

unsigned int CARD_GetLevel(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0xF00) >> 8;
}

unsigned int CARD_GetIcon(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0x7000) >> 12;
}

unsigned int CARD_GetType(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0xF8000) >> 15;
}

unsigned int CARD_GetRarity(unsigned short int CardID)
{
    return (CardProps[GetInternalID(CardID, MinCard_IntID)].Prop2 & 0xF00000) >> 20;
}

const char* ReturnMonsterTypeName(unsigned int Type)
{
    if (Type < MONSTER_TYPE_COUNT)
        return MonsterTypeNames[Type];
    return "Unknown";
}

unsigned short int GetMaxCardID()
{
    return CARD_GetCardID(IntIDs[IntIDCount - 1]);
}

unsigned short int GetMinCardID()
{
    return CARD_GetCardID(1);
}

int ParseCardNames(const char* IndexFilename, const char* NamesFilename, const char* DescFilename)
{
    FILE* findex = fopen(IndexFilename, "rb");
    if (!findex)
    {
        printf("ERROR: Can't open file %s for reading!\n", IndexFilename);
        perror("ERROR");
        return -1;
    }

    FILE* fnames = fopen(NamesFilename, "rb");
    if (!fnames)
    {
        printf("ERROR: Can't open file %s for reading!\n", NamesFilename);
        perror("ERROR");
        return -1;
    }

    FILE* fdesc = fopen(DescFilename, "rb");
    if (!fdesc)
    {
        printf("ERROR: Can't open file %s for reading!\n", DescFilename);
        perror("ERROR");
        return -1;
    }

    if (stat(IndexFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", IndexFilename);
        return -1;
    }

    CardIndex = (CardPair*)malloc(st.st_size);
    fread(CardIndex, st.st_size, 1, findex);
    fclose(findex);

    CardCount = st.st_size / 8;

    if (stat(NamesFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", NamesFilename);
        return -1;
    }

    StringBufferName = (wchar_t*)calloc(st.st_size, sizeof(char));
    fread(StringBufferName, sizeof(char), st.st_size, fnames);
    fclose(fnames);


    if (stat(DescFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", DescFilename);
        return -1;
    }

    StringBufferDesc = (wchar_t*)calloc(st.st_size, sizeof(char));
    fread(StringBufferDesc, sizeof(char), st.st_size, fdesc);
    fclose(fdesc);

    // shift indexes according to the memory locations...
    for (unsigned int i = 0; i < CardCount; i++)
    {
        CardIndex[i].Name = (wchar_t*)((int)StringBufferName + (int)CardIndex[i].Name);
        CardIndex[i].Description = (wchar_t*)((int)StringBufferDesc + (int)CardIndex[i].Description);
    }

    return 0;
}


int CreateCardNames(const char* IndexFilename, const char* NamesFilename, const char* DescFilename)
{
    unsigned int NameCursor = 8;
    unsigned int DescCursor = 8;

    unsigned int NamesSize = 0;
    unsigned int DescsSize = 0;

    CardIndex = (CardPair*)calloc(ImportedCardsCount + 1, sizeof(CardPair));
    CardIndex[0].Name = (wchar_t*)4;
    CardIndex[0].Description = (wchar_t*)4;

    for (unsigned int i = 0; i < ImportedCardsCount; i++)
    {
        CardIndex[i + 1].Name = (wchar_t*)NameCursor;
        CardIndex[i + 1].Description = (wchar_t*)DescCursor;
        NameCursor += ((wcslen(ImportDB[i].Name) + 1) * 2);
        NameCursor += NameCursor % 4; // align by 4 bytes - necessary for MIPS
        DescCursor += ((wcslen(ImportDB[i].Description) + 1) * 2);
        DescCursor += DescCursor % 4;
    }

    FILE* fnames = fopen(NamesFilename, "wb");
    if (!fnames)
    {
        printf("ERROR: Can't open file %s for writing!\n", NamesFilename);
        perror("ERROR");
        return -1;
    }

    for (unsigned int i = 0; i < ImportedCardsCount; i++)
    {
        fseek(fnames, (long)CardIndex[i + 1].Name, SEEK_SET);
        fwrite(ImportDB[i].Name, sizeof(wchar_t), wcslen(ImportDB[i].Name) + 1, fnames);
    }
    NamesSize = ftell(fnames);
    fclose(fnames);

    FILE* fdesc = fopen(DescFilename, "wb");
    if (!fdesc)
    {
        printf("ERROR: Can't open file %s for writing!\n", DescFilename);
        perror("ERROR");
        return -1;
    }

    for (unsigned int i = 0; i < ImportedCardsCount; i++)
    {
        fseek(fdesc, (long)CardIndex[i + 1].Description, SEEK_SET);
        fwrite(ImportDB[i].Description, sizeof(wchar_t), wcslen(ImportDB[i].Description) + 1, fdesc);
    }
    DescsSize = ftell(fdesc);
    fclose(fdesc);

    FILE* findex = fopen(IndexFilename, "wb");
    if (!findex)
    {
        printf("ERROR: Can't open file %s for writing!\n", IndexFilename);
        perror("ERROR");
        return -1;
    }
    fwrite(CardIndex, sizeof(CardPair), ImportedCardsCount + 1, findex);
    fwrite(&NamesSize, sizeof(int), 1, findex);
    fwrite(&DescsSize, sizeof(int), 1, findex);
    fclose(findex);


    return 0;
}

int SaveFiles()
{
    sprintf(FilePath, "%s\\CARD_IntID.bin", FolderName);
    CreateIntIDs(FilePath);

    sprintf(FilePath, "%s\\CARD_Pass.bin", FolderName);
    CreateCardPasswords(FilePath);

    sprintf(FilePath, "%s\\CARD_Prop.bin", FolderName);
    CreateCardProp(FilePath);

    sprintf(FilePath, "%s\\CARD_Indx_%s.bin", FolderName, Language);
    sprintf(FilePath2, "%s\\CARD_Name_%s.bin", FolderName, Language);
    sprintf(FilePath3, "%s\\CARD_Desc_%s.bin", FolderName, Language);

    CreateCardNames(FilePath, FilePath2, FilePath3);

    return 0;
}

int LoadFiles()
{
    sprintf(FilePath, "%s\\CARD_IntID.bin", FolderName);
    LoadIntIDs(FilePath);

    // card passwords are totally optional
    sprintf(FilePath, "%s\\CARD_Pass.bin", FolderName);
    if (bFileExists(FilePath))
        LoadCardPasswords(FilePath);
    else
        CardPasswords = (unsigned int*)calloc(IntIDCount, sizeof(unsigned int));

    sprintf(FilePath, "%s\\CARD_Prop.bin", FolderName);
    LoadCardProp(FilePath);

    sprintf(FilePath, "%s\\CARD_Indx_%s.bin", FolderName, Language);
    sprintf(FilePath2, "%s\\CARD_Name_%s.bin", FolderName, Language);
    sprintf(FilePath3, "%s\\CARD_Desc_%s.bin", FolderName, Language);
    ParseCardNames(FilePath, FilePath2, FilePath3);

    MinCard_ID = GetMinCardID();
    if (MinCard_ID == 4007) // we assume it's TF1
        MinCard_IntID = 4000;
    else
        MinCard_IntID = MinCard_ID;

    MaxCard_ID = GetMaxCardID();

    return 0;
}

int ExportCards(const char* OutFilename)
{
    FILE* fout = fopen(OutFilename, "wb");
    unsigned int CardWriteCounter = 0;
    unsigned int CardIDCounter = MinCard_IntID;

    if (!fout)
    {
        printf("ERROR: Can't open file %s for writing!\n", OutFilename);
        perror("ERROR");
        return -1;
    }

    // write BOM
    fputc(0xFF, fout);
    fputc(0xFE, fout);

    // not using card IDs for the loop to avoid missing cards...
    // using internal ids instead...
    //for (unsigned int i = MinCard_ID; i <= MaxCard_ID; i++)
    for (unsigned int i = 0; i < IntIDCount; i++)
    {
        if (GetInternalID(CardIDCounter, MinCard_IntID))
        {
            wprintf(L"Writing: [%d] %s\n", CardIDCounter, CARD_GetCardName(CardIDCounter));
            ReplaceCharsW(CardDescTempBuffer, CARD_GetCardDesc(CardIDCounter), '\n', '^');

            fwprintf(fout, L"[%d]\nName = %s\nDescription = %s\nATK = %d\nDEF = %d\nPassword = %d\nCardExistFlag = %d\nKind = %d\nAttr = %d\nLevel = %d\nIcon = %d\nType = %d\nRarity = %d\n\n", CardIDCounter, CARD_GetCardName(CardIDCounter), CardDescTempBuffer, CARD_GetAtk(CardIDCounter), CARD_GetDef(CardIDCounter), CARD_GetPassword(CardIDCounter), CARD_GetCardExistFlag(CardIDCounter), CARD_GetKind(CardIDCounter), CARD_GetAttr(CardIDCounter), CARD_GetLevel(CardIDCounter), CARD_GetIcon(CardIDCounter), CARD_GetType(CardIDCounter), CARD_GetRarity(CardIDCounter));
            CardWriteCounter++;
        }
        CardIDCounter++;
    }

    fclose(fout);
    printf("Total cards written: %d\n", CardWriteCounter);
    
    return 0;
}

int ImportCards(const char* InFilename)
{
    FILE* fin = fopen(InFilename, "rb");
    unsigned int MaxIDReadingPos = 2;
    wchar_t* MaxIDPos = NULL;
    wchar_t ReadCh = L'\0';
    unsigned int cursorpos = 0;
    unsigned int CardReadCounter = 0;
    wchar_t* parsercursor = NULL;
    wchar_t* endpoint = NULL;


    if (!fin)
    {
        printf("ERROR: Can't open file %s for reading!\n", InFilename);
        perror("ERROR");
        return -1;
    }

    // skip BOM
    fgetc(fin);
    fgetc(fin);

    // first, get min and max CardID nums (first and last) - this is important so we can properly generate and reference stuff...
    // for this reason we're gonna read the entire file to memory
    if (stat(InFilename, &st))
    {
        printf("ERROR: Can't find %s during size reading!\n", InFilename);
        return -1;
    }
    ImportedFile = malloc(st.st_size);
    ImportedFileSize = st.st_size - 2; // minus BOM

    fread(ImportedFile, sizeof(char), st.st_size - 2, fin);
    fclose(fin);

    // min number is easy - just get it off the top of the file...
    swscanf((wchar_t*)ImportedFile, L"[%d]\n", &MinCard_ID);

    // check for TF1 compatibility...
    if (MinCard_ID == 4007)
        MinCard_IntID = 4000;
    else
        MinCard_IntID = MinCard_ID;
    
    // max number is where things go just a little crazy - we're gonna read the file BACKWARDS until we find the '[' char so we can read it out
    for (MaxIDReadingPos = 2; MaxIDReadingPos < ImportedFileSize; MaxIDReadingPos += 2)
    {
        MaxIDPos = (wchar_t*)((int)(ImportedFile) + ImportedFileSize - MaxIDReadingPos);
        if (*MaxIDPos == L'[')
            break;
    }
    swscanf(MaxIDPos, L"[%d]\n", &MaxCard_ID);

    // next we need to count how many cards are in the file because we need to allocate space for it...
    // we do this by checking how many '[' are there in the file...
    while (cursorpos < ImportedFileSize)
    {
        ReadCh = *(wchar_t*)((int)(ImportedFile) + cursorpos);
        cursorpos += 2;
        if (ReadCh == L'[')
            ImportedCardsCount++;
    }

    // once we know how many there are, we can allocate...
    ImportDB = (CardImporter*)calloc(ImportedCardsCount, sizeof(CardImporter));

    // and finally start parsing the data
    // this parser stinks... it's a fixed format, so no regular .ini flexibility
    // this is because we're working with utf-16 and not utf-8 at the moment...

    parsercursor = (wchar_t*)ImportedFile;
    for (CardReadCounter = 0; CardReadCounter < ImportedCardsCount; CardReadCounter++)
    {
        // card id
        swscanf(parsercursor, L"[%d]\n", &ImportDB[CardReadCounter].CardID);

        // name
        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        endpoint = wcschr(parsercursor, '=') - 13;
        wcsncpy(CardNameTempBuffer, parsercursor, endpoint - parsercursor);
        ImportDB[CardReadCounter].Name = (wchar_t*)calloc(wcslen(CardNameTempBuffer) + 1, sizeof(wchar_t));
        wcscpy(ImportDB[CardReadCounter].Name, CardNameTempBuffer);
        
        // description
        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        endpoint = wcschr(parsercursor, '=') - 5;
        wcsncpy(CardDescTempBuffer, parsercursor, endpoint - parsercursor);
        ImportDB[CardReadCounter].Description = (wchar_t*)calloc(wcslen(CardDescTempBuffer) + 1, sizeof(wchar_t));
        ReplaceCharsW(ImportDB[CardReadCounter].Description, CardDescTempBuffer, '^', '\n');

        // ATK
        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].ATK);

        // DEF
        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].DEF);

        // Password
        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Password);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].CardExistFlag);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Kind);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Attr);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Level);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Icon);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Type);

        parsercursor = wcschr(parsercursor, '=');
        parsercursor += 2;
        swscanf(parsercursor, L"%d\n", &ImportDB[CardReadCounter].Rarity);

        parsercursor = wcschr(parsercursor, '[');

        memset(CardDescTempBuffer, 0, wcslen(CardDescTempBuffer) * 2);
        memset(CardNameTempBuffer, 0, wcslen(CardNameTempBuffer) * 2);
    }

    printf("Total cards read: %d\n", CardReadCounter);
    free(ImportedFile);

    return 0;
}

bool bCheckAttemptedLangDesignator(char* input)
{
    if ((input[0] == 'J') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'E') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'G') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'F') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'I') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'S') && (input[1] == '\0'))
        return true;


    if ((input[0] == 'j') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'e') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'g') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'f') && (input[1] == '\0'))
        return true;
    if ((input[0] == 'i') && (input[1] == '\0'))
        return true;
    if ((input[0] == 's') && (input[1] == '\0'))
        return true;

    return false;
}

int main(int argc, char* argv[])
{
    bool bSetLang = false;
    char* FolderCheckPoint = NULL;

    printf("Yu-Gi-Oh! Tag Force CardInfo Tool\n\n");

    if (argc < 2)
    {
        printf(TFCD_HELPMESSAGE, TFCD_HELPMESSAGE_VARS);
        return -1;
    }

    if (argv[1][0] == '-' && argv[1][1] == 'w') // Write mode
    {
        if (argc == 2)
        {
            printf(TFCD_HELPMESSAGE_WRITEMODE, argv[0]);
            return -1;
        }

        if (argc == 3)
        {
            strcpy(FolderName, argv[2]);
            FolderCheckPoint = strrchr(FolderName, '.');
            if (FolderCheckPoint)
                *FolderCheckPoint = 0;
        }
        else
        {
            if (bCheckAttemptedLangDesignator(argv[3]))
            {
                strcpy(Language, argv[3]);
                bSetLang = true;
            }
            else
                strcpy(FolderName, argv[3]);
        }

        if ((argc < 5) && !bSetLang)
            strcpy(Language, "E");
        else
            strcpy(Language, argv[4]);

        printf("Saving to folder: %s\n", FolderName);

        if (stat(FolderName, &st))
        {
            printf("Creating directory: %s\n", FolderName);
            mbstowcs(FolderNameWide, FolderName, 1024);
            _wmkdir(FolderNameWide);
        }

        printf("Importing card database to memory... This might take a bit.\n");
        ImportCards(argv[2]);

        printf("Generating files to designated folder...\n");
        SaveFiles();

        return 0;
    }

    printf("Using folder: %s\n", argv[1]);

    strcpy(FolderName, argv[1]);

    if (argc == 2)
    {
        strcpy(OutTextName, FolderName);
        strcat(OutTextName, ".ini");
    }
    else
    {
        // check if argv[2] is an attempted lang designator...
        // this normally wouldn't need to be done this way but since we're working with only a few args, it's not necessary
        if (bCheckAttemptedLangDesignator(argv[2]))
        {
            strcpy(Language, argv[2]);
            bSetLang = true;
            strcpy(OutTextName, FolderName);
            strcat(OutTextName, ".ini");
        }
        else
            strcpy(OutTextName, argv[2]);
    }

    if ((argc < 4) && !bSetLang)
        strcpy(Language, "E");
    else if (!bSetLang)
        strcpy(Language, argv[3]);

    printf("Loading card database to memory...\n");
    LoadFiles();

    printf("Exporting card database to: %s\n", OutTextName);
    ExportCards(OutTextName);

    return 0;
}

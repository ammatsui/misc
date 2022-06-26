#ifndef UTILS_H   
#define UTILS_H


#include <windows.h>
#include <iostream>
#include <math.h>


#define IDD_HAND_ABOUTBOX   103
#define IDD_FOOT_ABOUTBOX   110

/* for import table parsing */
HANDLE hFile;
HANDLE hMapObject;
LPVOID basePtr;
PIMAGE_FILE_HEADER     fileHeader;
PIMAGE_DOS_HEADER      dosHeader;
PIMAGE_NT_HEADERS      ntHeader;
PIMAGE_OPTIONAL_HEADER optionalHeader;
PIMAGE_SECTION_HEADER  sectionHeader;
PIMAGE_THUNK_DATA      thunkData;

/* for icon changing */
HANDLE hIcon;
HINSTANCE hExe;    
HRSRC hResource;    
HGLOBAL hMem;         
LPVOID lpResource;   
   

int import_directory_size;


std::string* get_DLL(LPCSTR fileName);

void set_ico(LPCSTR exeName, LPCSTR icoName);

float entropy(LPCSTR fileName);





/* parse import table */
std::string* get_DLL(LPCSTR fileName)
{
    hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (!hFile) 
    {
        printf("\nERROR : Could not open the file specified\n");
    }

    
    hMapObject = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    basePtr = MapViewOfFile(hMapObject, FILE_MAP_READ, 0, 0, 0);
       
    dosHeader = (PIMAGE_DOS_HEADER)basePtr;
          
    ntHeader = (PIMAGE_NT_HEADERS)((DWORD)basePtr + dosHeader->e_lfanew);
   
    fileHeader = (PIMAGE_FILE_HEADER)((DWORD)basePtr + dosHeader->e_lfanew + sizeof(ntHeader->Signature));
   
    DWORD numberofsections = fileHeader->NumberOfSections;

    DWORD RVAimport_directory = ntHeader->OptionalHeader.DataDirectory[1].VirtualAddress;

    sectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)ntHeader+sizeof(IMAGE_NT_HEADERS));

    PIMAGE_SECTION_HEADER import_section = {};
    DWORD section = 0;
    for(;section < numberofsections && 
    sectionHeader->VirtualAddress <= RVAimport_directory; sectionHeader++, section++);
    sectionHeader--;


    import_section = sectionHeader;

    DWORD import_table_offset = (DWORD)basePtr + import_section->PointerToRawData;
//imageBaseAddress + pointerToRawDataOfTheSectionContainingRVAofInterest + (RVAofInterest - SectionContainingRVAofInterest.VirtualAddress

    auto importImageDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(import_table_offset+(RVAimport_directory-sectionHeader->VirtualAddress));

    int import_directory_count = 0;
    for (;importImageDescriptor->Name != 0 ; importImageDescriptor++) 
    {
       
        thunkData = (PIMAGE_THUNK_DATA)(import_table_offset+(importImageDescriptor->FirstThunk - sectionHeader->VirtualAddress));
        import_directory_count++;
        for(;thunkData->u1.AddressOfData != 0; thunkData++)
        {
            import_directory_count++;
        }
   
    }

    auto importImDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(import_table_offset + (ntHeader->OptionalHeader.DataDirectory[1].VirtualAddress - import_section->VirtualAddress));
    std::string* res = new std::string[import_directory_count];
    int i = 0;

    for (;importImDescriptor->Name != 0 ; importImDescriptor++) 
    {
        DWORD Imported_DLL = import_table_offset + (importImDescriptor->Name - import_section->VirtualAddress);
        thunkData = (PIMAGE_THUNK_DATA)(import_table_offset + (importImDescriptor->FirstThunk - sectionHeader->VirtualAddress));
 
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%s", Imported_DLL);
        res[i] = tmp;

        i++;
        for(;thunkData->u1.AddressOfData != 0; thunkData++)
        {
            DWORD Imported_Fun = import_table_offset + (thunkData->u1.AddressOfData - sectionHeader->VirtualAddress + 2);// + 2);
            char temp[32];
            snprintf(temp, sizeof(temp), "%s", Imported_Fun);
            res[i] = temp;
            i++;
        }  
    }
    import_directory_size = import_directory_count;

    return res;

}


void set_ico(LPCSTR exeName, LPCSTR icoName)
{
    
    hIcon = LoadImage(NULL, icoName, 1, 0, 0, LR_LOADFROMFILE);

    
    hExe = LoadLibrary(exeName);
    if (hExe == NULL)
    {
        printf("Error loading module.\n");
        return;
    }
 
    hResource = FindResource(hExe, MAKEINTRESOURCE(440), RT_GROUP_ICON); 
 

 
    hMem = LoadResource(hExe, hResource); 
 
    lpResource = LockResource(hMem); 
 
    hMem = LoadResource(hExe, hResource); 
 
    lpResource = LockResource(hMem); 

    HANDLE hUpdateRes = BeginUpdateResource(exeName, TRUE);
    if (hUpdateRes == NULL)
    {
        printf("Could not open icon file for writing.\n");
        return ;
    }
    HANDLE ile = fopen(icoName, "rb");
    if (ile == NULL)
    {
        printf("could not open icon file.\n");
        return;
    }

    bool result = UpdateResource(hUpdateRes,
                        RT_ICON,
                        MAKEINTRESOURCE(440),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        lpResource,
                        sizeof(ile));
                         

    if (result == FALSE)
    {
        printf("Could not begin update resource.\n");
        return ;
    }


    if (!EndUpdateResource(hUpdateRes, FALSE))
    {
       printf("Could not end update resoruce.\n");
       return ;
    }
    
    printf("\nIcon update finished.\n\n");
    return ;

}


/* calculate entropy */
float entropy(LPCSTR fileName)
{
    long alphabet[256];
    unsigned char buffer[1024];
    int size = 0;
    float entropy = 0.0;
    memset(alphabet, 0, sizeof(long) * 256);

    FILE* pFile = fopen(fileName, "rb");
    if(pFile == NULL)
    {
        std::cout << "Failed to open\n";
    }

    int n;
    while((n = fread(buffer, 1, 1024, pFile)) != 0)
    {
        for (int i = 0; i < n; i++)
        {
            alphabet[(int) buffer[i]]++;
            size++;
        }
    }
    fclose(pFile);

    /* calculate entropy */
    float tmp;
    for (int i = 0; i < 256; i++)
    {
        if (alphabet[i] != 0)
        {
            tmp = (float) alphabet[i] / (float) size;
            entropy += -tmp * log2(tmp);
        }
    }
    return entropy;
}

#endif

#include <stdio.h>
#include <stdint.h>

// Partition 정보를 담아줄 구조체
struct MBR_Partitions
{
    uint8_t activeFlag[1];
    uint8_t CHSAddress1[3];
    uint8_t partitionType[1];
    uint8_t CHSAddress2[3];
    uint8_t LBSStartAddress[4];
    uint8_t partitionSize[4];
} typedef Partition;

// 엔디안 변환함수
void swapEndianness(void *data, int size)
{
    unsigned char *ptr = (unsigned char *)data;
    int i, j;
    unsigned char temp;

    for (i = 0, j = size - 1; i < j; i++, j--)
    {
        temp = ptr[i];
        ptr[i] = ptr[j];
        ptr[j] = temp;
    }
}

void extract_partition(Partition *ptr, FILE *f, uint64_t base)
{
    swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
    uint64_t LBSStartAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];

    printf("Real start address (DEC) : %lu", base + LBSStartAddress * 512);

    printf("\nFileType : ");
    unsigned char filetype[5];
    fseek(f, (unsigned char)LBSStartAddress * 512 + 3, SEEK_SET);
    fread(filetype, sizeof(unsigned char), 4, f);
    filetype[4] = '\0';
    printf("%s", filetype);

    
    swapEndianness(ptr->partitionSize, sizeof(ptr->partitionSize));
    uint64_t MBRSize = (ptr->partitionSize[0] << 24) + (ptr->partitionSize[1] << 16) + (ptr->partitionSize[2] << 8) + ptr->partitionSize[3];

    printf("\nfile size : %lu bytes\n", MBRSize * 512);
}

void extract_ebr_info(Partition *ptr, FILE *f, uint64_t EBRBaseAddress, uint64_t EBRStartAddress)
{
    printf("----------------------------------------\n");
    printf("EBR partition\n\n");
    fseek(f, 446, SEEK_CUR);
    unsigned char partition[16];
    fread(partition, sizeof(unsigned char), 16, f);
    ptr = (Partition *)(partition);

    extract_partition(ptr, f, EBRStartAddress);

    fseek(f, EBRStartAddress + 446 + 16, SEEK_SET);

    fread(partition, sizeof(unsigned char), 16, f);
    ptr = (Partition *)(partition);
    if (ptr->partitionType[0] != 0x05)
    {
        printf("\n\n[File type is not 0x05..]\n");
        printf("[EOF..]\n\n");
        return;
    }
    else{
        printf("\n\n[Next File type is 0x05..]\n");
        printf("[Continue parsing..]\n");
        swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
        uint64_t NextEBRAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];
        
        fseek(f, EBRBaseAddress + NextEBRAddress * 512, SEEK_SET); 
        extract_ebr_info(ptr, f, EBRBaseAddress, EBRBaseAddress + NextEBRAddress * 512);
    }
}

// MBR 파싱 함수
void extract_mbr_info(const char *image_path)
{
    FILE *f = fopen(image_path, "rb");
    if (f == NULL)
    {
        printf("파일을 찾을 수 없습니다.\n");
        return;
    }

    unsigned char partition[16];
    Partition *ptr;

    fseek(f, 446, SEEK_SET);

    for (int i = 1; 1; i++)
    {
        fread(partition, sizeof(unsigned char), 16, f);

        ptr = (Partition *)(partition);
        
        if (ptr->partitionType[0] != 0x07)
        {
            break;
        }
        printf("----------------------------------------\n");
        printf("Partition #%d\n", i);
        extract_partition(ptr, f, 0);

        fseek(f, 446 + 16 * i, SEEK_SET);
    }
    // Partition이 4개보다 많기에 EBR을 통해 관리하는 디스크임을 알 수 있음
    if (ptr->partitionType[0] == 0x05)
    {
        printf("----------------------------------------\n");
        printf("Partition #4\n");
        printf("\n\nEBR partition start..\n");

        swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
        uint64_t LBSStartAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];
        
        LBSStartAddress = LBSStartAddress * 512;

        fseek(f, LBSStartAddress, SEEK_SET);
        extract_ebr_info(ptr, f, LBSStartAddress, LBSStartAddress);
    }
}

int main(int argc, char* argv[])
{
    // 이미지 파일 경로를 파라미터로 받아서 함수 호출
    const char *image_path = argv[1];
    extract_mbr_info(image_path);

    return 0;
}
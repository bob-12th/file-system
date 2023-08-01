#include <stdio.h>
#include <stdint.h>
struct MBR
{
    uint8_t activeFlag[1];
    uint8_t CHSAddress1[3];
    uint8_t partitionType[1];
    uint8_t CHSAddress2[3];
    uint8_t LBSStartAddress[4];
    uint8_t partitionSize[4];
} typedef Partition;

// 주소 계산을 위해 byte 배열을 64bit int로 변환해주는 함수 구현
int32_t bytesToInt64(const unsigned char *byteArray)
{
    int32_t result = 0;

    result |= ((int32_t)byteArray[0]) << 24;
    result |= ((int32_t)byteArray[1]) << 16;
    result |= ((int32_t)byteArray[2]) << 8;
    result |= ((int32_t)byteArray[3]);
    return result;
}

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
    /* printf("active or not : %02x\n", (unsigned char)ptr->activeFlag[0]);

    printf("LBS Address of start (before endian swap): ");
    for (int i = 0; i < 4; i++)
    {
        printf("%02x ", (unsigned char)ptr->LBSStartAddress[i]);
    }
*/
    swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
    uint64_t LBSStartAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];
    /*
        printf("\nLBS Address of start (after endian swap): ");
        for (int i = 0; i < 4; i++)
        {
            printf("%02x ", (unsigned char)ptr->LBSStartAddress[i]);
        }
    */
    // int32_t LBSStartAddress = bytesToInt64(ptr->LBSStartAddress);
    // uint32_t LBSStartAddress = ptr->LBSStartAddress[0] + (ptr->LBSStartAddress[1] << 8) + (ptr->LBSStartAddress[2] << 16) + (ptr->LBSStartAddress[3] << 24);
    // uint32_t LBSStartAddress = *(uint32_t *)ptr->LBSStartAddress;

    printf("Real start address (DEC) : %lu", base + LBSStartAddress * 512);
    printf("\nFileType : ");
    unsigned char filetype[5];
    fseek(f, (unsigned char)LBSStartAddress * 512 + 3, SEEK_SET);
    fread(filetype, sizeof(unsigned char), 4, f);
    filetype[4] = '\0';
    printf("%s", filetype);

    // for (int i = 0 ; i < 4 ; i++)
    // {
    //     printf("%02x ",(unsigned char)ptr->partitionSize[i]);
    // }
    swapEndianness(ptr->partitionSize, sizeof(ptr->partitionSize));
    // int32_t MBRSize = bytesToInt64(ptr->partitionSize);
    uint64_t MBRSize = (ptr->partitionSize[0] << 24) + (ptr->partitionSize[1] << 16) + (ptr->partitionSize[2] << 8) + ptr->partitionSize[3];

    printf("\nfile size : %lu bytes\n", MBRSize * 512);
}

void extract_ebr_info(Partition *ptr, FILE *f, uint64_t EBRBaseAddress, uint64_t EBRStartAddress)
{
    printf("----------------------------------------\n");
    fseek(f, 446, SEEK_CUR);
    unsigned char partition[16];
    fread(partition, sizeof(unsigned char), 16, f);
    ptr = (Partition *)(partition);

    extract_partition(ptr, f, EBRStartAddress);

    fseek(f, EBRStartAddress + 446 + 16, SEEK_SET);

    // swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
    // uint64_t NextEBRAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];
    // printf("change : %lld\n",NextEBRAddress);
    // printf("asdf : %lld\n",FirstEBRAddress + 16 * i);
    // printf("? : %02x",ptr->partitionType[0]);
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
        
        // printf("NextEBRAddress : %lld\n",NextEBRAddress);
        // printf("%lld\n",EBRBaseAddress + NextEBRAddress * 512);
        // printf("EBRStartAddress : %lld\n",EBRBaseAddress);
        
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
        // printf("type : %02x",ptr->partitionType[0]);
        if (ptr->partitionType[0] != 0x07)
        {
            break;
        }
        printf("----------------------------------------\n");
        printf("Partition #%d\n", i);
        extract_partition(ptr, f, 0);

        // printf("iter %d\n\n", i);

        fseek(f, 446 + 16 * i, SEEK_SET);
    }
    // Partition이 4개보다 많기에 EBR을 통해 관리하는 디스크임을 알 수 있음
    if (ptr->partitionType[0] == 0x05)
    {
        printf("----------------------------------------\n");
        printf("Partition #4\n");
        printf("\n\nEBR partition start..\n");
        // for (int i = 0; i < 4; i++)
        // {
        //     printf("%02x ", ptr->LBSStartAddress[i]);
        // }
        // printf("\n\n");
        swapEndianness(ptr->LBSStartAddress, sizeof(ptr->LBSStartAddress));
        // for (int i = 0; i < 4; i++)
        // {
        //     printf("%02x ", ptr->LBSStartAddress[i]);
        // }
        uint64_t LBSStartAddress = (ptr->LBSStartAddress[0] << 24) + (ptr->LBSStartAddress[1] << 16) + (ptr->LBSStartAddress[2] << 8) + ptr->LBSStartAddress[3];
        LBSStartAddress = LBSStartAddress * 512;

        fseek(f, LBSStartAddress, SEEK_SET);
        extract_ebr_info(ptr, f, LBSStartAddress, LBSStartAddress);
    }
}

int main()
{
    // 이미지 파일 경로를 지정하여 함수를 호출
    const char *image_path = "mbr_128.dd";
    extract_mbr_info(image_path);

    return 0;
}
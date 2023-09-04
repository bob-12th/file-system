#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct fat32{
    unsigned char JumpBootCode[3];
    unsigned char OEMName[8];
    unsigned char BytesPerSector[2];
    unsigned char SectorPerCluster[1];
    unsigned char ReservedSectorCount[2];
}typedef FAT;

// 엔디안 변환 함수
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

// 0f ff ff ff를 만나면 종료
int checkEndOfChain(unsigned char* block)
{
    unsigned char ff[] = {0x0F, 0xFF, 0xFF, 0xFF};
    int cnt = 0;
    for(int i = 0 ; i < 4 ; i++)
    {
        if(block[i] == ff[i])
        {
            cnt++;
            if(cnt == 4) return 1;
        }
    }
    return 0;
}

// 파싱 함수
void extractFATTable(FILE* f, int startIndex)
{
    printf("Start extracting FAT Table..\n\n");
    unsigned char temp[16];
    FAT* ptr;
    fread(temp, sizeof(unsigned char), 16, f);
    ptr = (FAT* )(temp);

    int ReservedSectorCount = (ptr->ReservedSectorCount[1] << 8) | ptr->ReservedSectorCount[0];
    int FATStartAddress = ReservedSectorCount * 512;
    // Reserved 영역 * 512를 하여 실제 FAT Table의 주소에 접근한다
    printf("FAT Table Start Address : %02x..\n\n",FATStartAddress);

    fseek(f, FATStartAddress + startIndex * 4, SEEK_SET);

    // 4 byte씩 끊어 읽을 배열
    unsigned char block[4];

    printf("hex / dec\n");
    
    for(int k = 0 ; k < 319 ; k++)
    {
        // 읽고 엔디안 변환
        fread(block, sizeof(unsigned char), 4, f);
        swapEndianness(block, 4);
        
        printf("\n");
        // 0f ff ff ff 인지 체크
        if(checkEndOfChain(block))
        {
            printf("\n");
            return;
        }
        // 아니라면 hex값 출력
        for(int i = 0 ; i < 4 ; i++)
        {
            printf("%02x ",block[i]);
        }
        // 그리고 다음 주소로 분기하기 위해 int 변환
        uint64_t address = (block[0] << 24) + (block[1] << 16) + (block[2] << 8) + block[3];
        // printf("%ld \n",(ftell(f) - FATStartAddress) / 4 - 1);
        printf("/ %ld ",address);

        // 다음 주소로 분기
        fseek(f, FATStartAddress + address * 4 , SEEK_SET);
;    }
}

// main
int main(int argc, char* argv[]){
    const char* image_path = "fat32.dd";
    FILE *f = fopen(image_path, "rb");
    if (f == NULL)
    {
        printf("파일을 찾을 수 없습니다.\n");
        return 0;
    }
    int startIndex = atoi(argv[1]);
    printf("Start Cluster : %d..\n\n",startIndex);
    extractFATTable(f, startIndex);
    
    fclose(f);
    return 0;
}
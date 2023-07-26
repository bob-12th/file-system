#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Partition 정보들을 담을 구조체 선언
struct Partition
{
    char TypeGUID[16];
    char UniqGUID[16];
    char FirstLBA[8];
    char LastLBA[8];
    char attributeFlags[8];
    char PartitionName[72];
};

// 주소 계산을 위해 byte 배열을 64bit int로 변환해주는 함수 구현
int64_t bytesToInt64(const char *bytes)
{
    int64_t result = 0;
    for (int i = 0; i < 8; ++i)
    {
        result |= ((int64_t)(bytes[i] & 0xFF)) << (8 * i);
    }
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

// GPT 파싱 함수
void extract_gpt_info(const char *image_path)
{
    FILE *f = fopen(image_path, "rb");
    if (f == NULL)
    {
        printf("파일을 찾을 수 없습니다.\n");
        return;
    }

    // 파티션으로 분기
    fseek(f, 1024, SEEK_SET);
    
    // 최대 파티션 개수만큼 돌기
    for (int cnt = 1 ; cnt < 129 ; cnt++)
    {
        printf("Partition %d\n",cnt);
        unsigned char partition[128];

        fread(partition, sizeof(unsigned char), 128, f);

        struct Partition *ptr = (struct Partition *)(partition);

        // 파티션이 더이상 없으면 종료
        if(ptr->TypeGUID[0] == 0x00)
        {
            printf("Partition %d doesn't exists..\n",cnt);
            printf("End of Program..\n");
            break;
        }

        // 주소 연산을 위해 64bit 정수 변수 정의
        int64_t firstLBA_integer = bytesToInt64(ptr->FirstLBA);
        int64_t lastLBA_integer = bytesToInt64(ptr->LastLBA);

        // 각각의 정보 출력
        printf("TypeGUID\n>> ");
        for (int i = 0; i < 16; i++)
        {
            printf("%02x ", (unsigned char)ptr->TypeGUID[i]);
        }
        printf("\n");

        printf("UniqGUID\n>> ");
        for (int i = 0; i < 16; i++)
        {
            printf("%02x ", (unsigned char)ptr->UniqGUID[i]);
        }
        printf("\n");
        
        printf("FileType\n>> ");
        unsigned char filetype[5];
        fseek(f,(unsigned char)firstLBA_integer * 512 + 3 , SEEK_SET);
        fread(filetype, sizeof(unsigned char), 4, f);
        filetype[5] = '\0';
        printf("%s",filetype);
        printf("\n");

        printf("FirstLBA\n>> ");
        for (int i = 0; i < 8; i++)
        {
            printf("%02x ", (unsigned char)ptr->FirstLBA[i]);
        }
        printf("\n");

        printf("LastLBA\n>> ");
        for (int i = 0; i < 8; i++)
        {
            printf("%02x ", (unsigned char)ptr->LastLBA[i]);
        }
        printf("\n");

        printf("Size\n>> %ld\n\n", (lastLBA_integer - firstLBA_integer) * 512);

        // filetype을 파싱하기 위해 움직였던 파일 읽기 포인터를 다은 파티션을 가리키게
        fseek(f, 1024 + 128 * cnt, SEEK_SET);
    }
    printf("\n");

    fclose(f);
}

int main()
{
    // 이미지 파일 경로를 지정하여 함수를 호출
    const char *image_path = "gpt_128.dd";
    extract_gpt_info(image_path);

    return 0;
}

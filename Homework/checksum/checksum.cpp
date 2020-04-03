#include <stdint.h>
#include <stdlib.h>

/**
 * @brief 进行 IP 头的校验和的验证
 * @param packet 完整的 IP 头和载荷
 * @param len 即 packet 的长度，单位是字节，保证包含完整的 IP 头
 * @return 校验和无误则返回 true ，有误则返回 false
 */
bool validateIPChecksum(uint8_t *packet, size_t len) {
        // TODO:
        int sum=0;
        int head_length = packet[0]&(0xf);
        for (int i=0;i<4*head_length;i+=2)
        {
                long temp=0,temp2=(long)packet[i];
                
                temp+=temp2*256;
                sum+=temp+packet[i+1];
        }
        int high_16 = (sum>>16);
        while(high_16)
        {
                sum = sum&(0xffff);
                sum += high_16;
                high_16 = (sum>>16);
        }
        sum =(~sum)&0xffff;
        if (!sum)
                return true;
        else return false;
}
